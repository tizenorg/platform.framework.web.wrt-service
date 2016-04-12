
ADD_DEFINITIONS("-fPIC")

SET(NODE_PLUGIN_PATH /usr/lib/node/wrt-service)

#################################### native #####################################

PKG_CHECK_MODULES(native_pkgs REQUIRED dlog nodejs)

INCLUDE_DIRECTORIES (
    extension
    ${native_pkgs_INCLUDE_DIRS}
)

SET(SRCS
    native-node.cpp
    extension/extension.cpp
    extension/extension-manager.cpp
    extension/extension-adapter.cpp
)

ADD_LIBRARY(${TARGET_NODE_NATIVE} MODULE ${SRCS})

SET_TARGET_PROPERTIES(${TARGET_NODE_NATIVE} PROPERTIES PREFIX "" SUFFIX ".node")
TARGET_LINK_LIBRARIES(${TARGET_NODE_NATIVE} ${native_pkgs_LIBRARIES})
INSTALL(TARGETS ${TARGET_NODE_NATIVE} DESTINATION ${NODE_PLUGIN_PATH})

#################################### gcontext #####################################

PKG_CHECK_MODULES(gcontext_pkgs REQUIRED nodejs glib-2.0 gobject-2.0)

INCLUDE_DIRECTORIES (
    ${gcontext_pkgs_INCLUDE_DIRS}
)

SET(SRCS
    gcontext.cpp
)

ADD_LIBRARY(${TARGET_NODE_GCONTEXT} MODULE ${SRCS})

SET_TARGET_PROPERTIES(${TARGET_NODE_GCONTEXT} PROPERTIES PREFIX "" SUFFIX ".node")
TARGET_LINK_LIBRARIES(${TARGET_NODE_GCONTEXT} ${gcontext_pkgs_LIBRARIES})
INSTALL(TARGETS ${TARGET_NODE_GCONTEXT} DESTINATION ${NODE_PLUGIN_PATH})

#################################### appfw #########################################

PKG_CHECK_MODULES(appfw_pkgs REQUIRED dlog nodejs glib-2.0 aul)

INCLUDE_DIRECTORIES (
    ${appfw_pkgs_INCLUDE_DIRS}
)

SET(SRCS
    tizen-appfw.cpp
)

ADD_LIBRARY(${TARGET_NODE_APPFW} MODULE ${SRCS})

SET_TARGET_PROPERTIES(${TARGET_NODE_APPFW} PROPERTIES PREFIX "" SUFFIX ".node")
TARGET_LINK_LIBRARIES(${TARGET_NODE_APPFW} ${appfw_pkgs_LIBRARIES})
INSTALL(TARGETS ${TARGET_NODE_APPFW} DESTINATION ${NODE_PLUGIN_PATH})


#################################### serviceutil #########################################
PKG_CHECK_MODULES(serviceutil_pkgs REQUIRED dlog nodejs sqlite3)
PKG_CHECK_MODULES(security_pkgs REQUIRED security-manager)
PKG_CHECK_MODULES(manifest_pkgs REQUIRED manifest-parser wgt-manifest-handlers)
INCLUDE_DIRECTORIES (
    ${serviceutil_pkgs_INCLUDE_DIRS}
    ${security_pkgs_INCLUDE_DIRS}
    ${manifest_pkgs_INCLUDE_DIRS}
)

SET(SRCS
    service-util.cpp
)

ADD_LIBRARY(${TARGET_NODE_SERVICEUTIL} MODULE ${SRCS})

SET_TARGET_PROPERTIES(${TARGET_NODE_SERVICEUTIL} PROPERTIES PREFIX "" SUFFIX ".node")
TARGET_LINK_LIBRARIES(${TARGET_NODE_SERVICEUTIL} ${security_pkgs_LIBRARIES} ${serviceutil_pkgs_LIBRARIES} ${manifest_pkgs_LIBRARIES})
INSTALL(TARGETS ${TARGET_NODE_SERVICEUTIL} DESTINATION ${NODE_PLUGIN_PATH})

#################################### dlog #########################################
PKG_CHECK_MODULES(nodedlog_pkgs REQUIRED dlog nodejs)
INCLUDE_DIRECTORIES (
    ${nodedlog_pkgs_INCLUDE_DIRS}
)

SET(SRCS
    dlog.cpp
)

ADD_LIBRARY(${TARGET_NODE_DLOG} MODULE ${SRCS})

SET_TARGET_PROPERTIES(${TARGET_NODE_DLOG} PROPERTIES PREFIX "" SUFFIX ".node")
TARGET_LINK_LIBRARIES(${TARGET_NODE_DLOG} ${nodedlog_pkgs_LIBRARIES})
INSTALL(TARGETS ${TARGET_NODE_DLOG} DESTINATION ${NODE_PLUGIN_PATH})

