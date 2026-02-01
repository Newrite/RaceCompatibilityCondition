-- set minimum xmake version
set_xmakever("3.0.5")

-- includes
includes(os.getenv("CommonLibSSE-NG"))

-- set project
set_project("RaceCompatibilityCondition")
set_version("1.0.3")
set_license("GPL-3.0")

-- set defaults
set_languages("c++latest")
set_warnings("allextra")
set_defaultmode("releasedbg")

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- set policies
set_policy("package.requires_lock", true)

-- set configs
set_config("skyrim_vr", true)
set_config("skyrim_ae", true)
set_config("skyrim_se", true)
set_config("skse_xbyak", true)

-- targets
target("RaceCompatibilityCondition")
    -- add dependencies to target
    add_deps("commonlibsse-ng")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "RaceCompatibilityCondition",
        author = "Newrite",
        description = "SKSE64 plugin template workaround about Race Compatibility"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_headerfiles("src/**.hpp")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

