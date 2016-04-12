#!/usr/bin/env node

/* global GLOBAL, require */

(function(){
    var path = require('path');

    /**
     * String key-value pairs to provide runtime information to extensions.
     *
     * @private
     */
    var runtime_variables_ = {'runtime_name': 'wrt-service'};

    /**
     * Cached result of native.getExtensions() indexed by extension name, eg:
     *
     * {'tizen.contact': {
     *   name: 'tizen.contact',
     *   jsapi: '....', // extension JS code as string
     *   entry_points: [], // list of extension additional entry points, eg. tizen.AddressBook
     *   loaded: false // this will be true if 'jsapi' was loaded through eval()
     * }}
     *
     * @private
     */
    var extensions_ = {};

    /**
     * Used to provide namespaces as readonly properties, eg.
     * Object.defineProperty(GLOBAL.tizen, 'contact', {
     *   get: function () {
     *     return _api.tizen.contact,
     *   },
     *   configurable: false,
     *   enumerable: true
     * });
     *
     * @private
     */
    var api_ = {};

    var ExtensionLoader = function() {
        native.initialize(runtime_variables_);

        var extensions = native.getExtensions();
        for (var i = 0; i < extensions.length; i++) {
            extensions[i].loaded = false;
            extensions_[extensions[i].name] = extensions[i];
        }
        for (var name in extensions_) {
            if (extensions_[name].use_trampoline) {
                this.installTrampoline(extensions_[name]);
            } else {
                this.load(extensions_[name]);
            }
        }
    };

    ExtensionLoader.prototype = {
        /**
         * Creates namespace for 'name' in given object.
         * Eg. this.createNamespace(GLOBAL, 'tizen.contact') will create:
         * GLOBAL.tizen.contact = {}
         *
         * @param {Object} object
         * @param {String} name
         */
        createNamespace: function(object, name) {
            var obj = object;
            var arr = name.split('.');
            for (var i = 0; i < arr.length; i++) {
                obj[arr[i]] = obj[arr[i]] || {};
                obj = obj[arr[i]];
            }
        },
        exposeApi: function (ext) {

            var i, entry_points, entry_point, tmp, parent_name, base_name;

            // additional entry points are installed in GLOBAL context by eval()
            // so we need to move it to protected api_ object first
            entry_points = ext.entry_points;
            for (i = 0; i < entry_points.length; i++) {
                entry_point = entry_points[i];
                tmp = entry_point.split('.');
                parent_name = tmp[0];
                base_name = tmp[tmp.length - 1];

                api_[parent_name][base_name] = GLOBAL[parent_name][base_name];
                delete GLOBAL[parent_name][base_name];
            }

            entry_points.push(ext.name);

            for (i = 0; i < entry_points.length; i++) {
                entry_point = entry_points[i];
                tmp = entry_point.split('.');
                parent_name = tmp[0];
                base_name = tmp[tmp.length - 1];

                Object.defineProperty(GLOBAL[parent_name], base_name, {
                    get: function (parent_name, base_name) {
                        return function () {
                            return api_[parent_name][base_name];
                        }
                    }(parent_name, base_name),
                    configurable: false,
                    enumerable: true
                });
            }
        },

        /**
         * @param {Object} ext
         */
        load: function(ext) {
            if (ext.loaded) {
                return;
            }

            ext.loadInstance();

            ext.loaded = true;

            this.createNamespace(api_, ext.name);
            this.createNamespace(GLOBAL, ext.name);

            var api = (ext.use_trampoline) ? api_ : GLOBAL;

            var jscode =
                '(function(extension) {' +
                '  extension.internal = {};' +
                '  extension.internal.sendSyncMessage = extension.sendSyncMessage;' +
                '  delete extension.sendSyncMessage;' +
                '  var exports = {}; ' +
                '  (function() {\'use strict\'; ' + ext.jsapi + '})();' +
                '  api.' + ext.name + ' = exports; ' +
                '});';

            try {
                var func = eval(jscode);
                func({
                    postMessage: function(msg) {
                        return ext.postMessage(msg);
                    },
                    sendSyncMessage: function(msg) {
                        return ext.sendSyncMessage(msg);
                    },
                    setMessageListener: function(fn) {
                        return ext.setMessageListener(fn);
                    },
                    sendRuntimeMessage: function(type, data) {
                        return runtimeMessageHandler(type, data);
                    },
                    sendRuntimeSyncMessage: function(type, data) {
                        return runtimeMessageHandler(type, data);
                    },
                    sendRuntimeAsyncMessage: function(type, data, callback) {
                        return runtimeMessageHandler(type, data, callback);
                    },
                    postData: function(msg, chunk) {
                        return ext.postData(msg, chunk);
                    },
                    sendSyncData: function(msg, chunk) {
                        return ext.sendSyncData(msg, chunk);
                    },
                    setDataListener: function(fn) {
                        return ext.setDataListener(fn);
                    },
                    receiveChunkData: function(id, type) {
                        return ext.receiveChunkData(id, type);
                    }
                });

                if (ext.use_trampoline) {
                    this.exposeApi(ext);
                }
            } catch (err) {
                dlog.loge('Error loading extension "' + ext.name + '": ' + err.message);
            }
        },

        /**
         * This is used to defer extension loading to it's first usage.
         * Eg. First access to tizen.contact will load extension's 'jsapi' through eval().
         *
         * @param {Object} ext
         */
        installTrampoline: function(ext) {

            var entry_points = ext.entry_points;
            entry_points.push(ext.name);

            for (var i = 0; i < entry_points.length; i++) {
                var tmp = entry_points[i].split('.');
                var parent_name = tmp[0];
                var base_name = tmp[tmp.length - 1];

                this.createNamespace(GLOBAL, entry_points[i]);

                Object.defineProperty(GLOBAL[parent_name], base_name, {
                    get: function (parent_name, base_name) {
                        return function() {
                            try {
                                this.deleteTrampoline(ext);
                                this.load(ext);
                                return api_[parent_name][base_name];
                            } catch (e) {
                                console.log(e.stack);
                            }
                        }.bind(this);
                    }.call(this, parent_name, base_name),
                    enumerable: true
                });
            }

        },

        deleteTrampoline: function(ext) {
            var entry_points = ext.entry_points;
            entry_points.push(ext.name);

            for (var i = 0; i < entry_points.length; i++) {
                var tmp = entry_points[i].split('.');
                var parent_name = tmp[0];
                var base_name = tmp[tmp.length - 1];
                delete GLOBAL[parent_name][base_name];
            }
        }
    };

    //change cmdline
    process.title = process.argv[1];
    var cmdpath =  process.argv[1].split('/');

    //getting appid from cmdpath
    var appid = cmdpath.pop();
    cmdpath.pop();

    //getting package path
    var packagePath = cmdpath.join('/');

    //Change App privilege
    var serviceUtil = require('wrt-service/serviceutil.node');
    serviceUtil.setPrivilege(appid.split('.')[0]);

    // change cwd
    process.chdir(packagePath);
    process.chdir('res/wgt');

    //initialize internal modules

    //dlog enable
    var util = require('util');
    var dlog = require('wrt-service/nodedlog.node');
    console.log = function(){
        dlog.logd(appid, util.format.apply(this, arguments));
    };
    console.info = function(){
        dlog.logv(appid, util.format.apply(this, arguments));
    };
    console.error = function(){
        dlog.loge(appid, util.format.apply(this, arguments));
    };
    console.warn = console.info;
    console.logd = function(){
        if( arguments.length > 1 ){
            dlog.logd(arguments[0], util.format.apply(this,  Array.prototype.slice.call(arguments,1)) );
        }else{
            dlog.logd(util.format.apply(this, arguments) );
        }
    };
    console.logv = function(){
        if( arguments.length > 1 ){
            dlog.logv(arguments[0], util.format.apply(this,  Array.prototype.slice.call(arguments,1)) );
        }else{
            dlog.logv(util.format.apply(this, arguments) );
        }
    };
    console.loge = function(){
        if( arguments.length > 1 ){
            dlog.loge(arguments[0], util.format.apply(this,  Array.prototype.slice.call(arguments,1)) );
        }else{
            dlog.loge(util.format.apply(this, arguments) );
        }
    };

    //init g main loop
    require('wrt-service/gcontext.node').init();

    //appfw load
    var appfw = require('wrt-service/appfw.node');
    //native load
    var native = require('wrt-service/native.node');
    //ACE module load
    serviceUtil.initAce(appid);

    runtime_variables_['app_id'] = appid;
    new ExtensionLoader();

    //module path update for custom modules
    var modulepaths = [ process.cwd(), process.cwd() + '/node_modules' ];
    module.paths = modulepaths.concat(module.paths);

    //getting start page
    var startScript = serviceUtil.getStartScript(appid, packagePath);
    if (!startScript) {
        startScript = "service.js";
    }

    //load user script
    var app = require( path.join(packagePath, '/res/wgt/' + startScript));

    appfw.init(process.argv.slice(1));
    appfw.onService = function(bundle) {
        runtime_variables_['encoded_bundle'] = bundle;
        native.updateRuntimeVariables(runtime_variables_);
        if (app.onRequest) {
            app.onRequest();
        }
    };
    appfw.onTerminate = function() { process.exit(); };

    var runtimeMessageHandler = function(type, data, callback) {
        if (type === 'tizen://exit') {
            appfw.onTerminate();
        }
    }

    if (app.onExit) {
        process.on('exit',app.onExit);
    }

    if (app.onStart) {
        app.onStart();
    }

    //running main loop

})();
