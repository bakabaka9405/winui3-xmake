package("microtex")
    set_homepage("https://github.com/NanoMichael/MicroTeX")
    set_description("A lightweight LaTeX rendering library")
    set_license("MIT")

    add_urls("https://github.com/NanoMichael/MicroTeX.git")
    add_versions("0.1.0", "0e3707f6dafebb121d98b53c64364d16fefe481d")

    add_deps("cmake", "tinyxml2")
    add_includedirs("include")
    add_links("LaTeX")
    add_syslinks("d2d1", "dwrite")
    add_defines("BUILD_WIN32", "_HAS_STD_BYTE=0")

    on_install("windows", function (package)
        io.writefile("CMakeLists.txt", io.readfile("CMakeLists.txt") .. [[

install(TARGETS LaTeX ARCHIVE DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/" DESTINATION include)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/" DESTINATION res)
]])

        import("package.tools.cmake").install(package, {
            "-DBUILD_SHARED_LIBS=OFF",
            "-DHAVE_LOG=OFF",
            "-DGRAPHICS_DEBUG=OFF",
            "-DMEM_CHECK=OFF",
            "-DQT=OFF",
            "-DSKIA=OFF",
            "-DBUILD_EXAMPLE=OFF"
        }, {packagedeps = "tinyxml2"})
    end)
