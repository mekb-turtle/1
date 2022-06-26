workspace("1")
configurations({ "Debug", "Release" })

project("1")
kind("ConsoleApp")
language("C")
targetdir("bin/%{cfg.buildcfg}")

files({ "**.h", "**.c" })

filter("configurations:Debug")
defines({ "DEBUG" })
symbols("On")

filter("configurations:Release")
defines({ "NDEBUG" })
optimize("On")
