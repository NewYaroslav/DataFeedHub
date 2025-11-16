include_guard(GLOBAL)

# Resolve effective dependency mode for a component.
# If cache variable is unset or INHERIT, fallback to DFH_DEPS_MODE.
function(dfh_get_effective_mode cache_var out_var)
    if(NOT DEFINED ${cache_var})
        set(${out_var} "${DFH_DEPS_MODE}" PARENT_SCOPE)
        return()
    endif()
    set(_value "${${cache_var}}")
    if(_value STREQUAL "" OR _value STREQUAL "INHERIT")
        set(_value "${DFH_DEPS_MODE}")
    endif()
    set(${out_var} "${_value}" PARENT_SCOPE)
endfunction()
