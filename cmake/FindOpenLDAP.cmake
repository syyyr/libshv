find_path(OPENLDAP_INCLUDE_DIR ldap.h PATHS
        /usr/include
        /opt/local/include
        /usr/local/include
        )
find_library(OPENLDAP_LIBRARY ldap)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenLDAP
        DEFAULT_MSG
        OPENLDAP_INCLUDE_DIR
        OPENLDAP_LIBRARY
        )
mark_as_advanced(
        OPENLDAP_INCLUDE_DIR
        OPENLDAP_LIBRARY
        )

add_library(OpenLDAP::OpenLDAP UNKNOWN IMPORTED)
set_target_properties(OpenLDAP::OpenLDAP PROPERTIES
        IMPORTED_LOCATION "${OPENLDAP_LIBRARY}"
        )
target_include_directories(OpenLDAP::OpenLDAP INTERFACE "${OPENLDAP_INCLUDE_DIR}")
