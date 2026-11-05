# SaxonC-HE 13.0.0 download:
SaxonC-HE is a compiled variant of Saxon from the Java
platform to the C/C++ platform using GraalVM `native-image`.

SaxonC-HE provides processing in XSLT 3.0, XQuery 3.1 and XPath 3.1, and,
in SaxonC-EE, Schema validation 1.0/1.1.

## What's included in this download?
In this download, the binary shared libraries (.dll files) for the
core part of Saxon, compiled with `native-image`, which we call
`libsaxonc-core-he`, along with the sources for the C/C++-specific API,
which we call `libsaxonc-he`, and its binary shared libraries, along with
the Transform and Query binaries for running XSLT and XQuery from the
command line.

## Installation
Installation on Windows is a matter of extracting the contents of the download `.zip` to a suitable location. We recommend `C:\Program Files\Saxonica\SaxonCHE-windows-x86_64-13-0-0`

### Setting up the PATH to include the SaxonC DLLs
If you're intending to use the pre-compiled binaries, like Transform and Query,
then you need to ensure that they can find all the DLLs they need on your PATH.

There are several ways to do this. The most permanent is by setting them
through the Environment Variables control panel -- on Windows 11, the simplest
way to locate this is to search for "Environment variables" in the start menu,
or in the Settings panel. "Edit environment variables for your account" should
be what you want, unless you have a clear reason to edit the system environment
variables instead. Assuming you've unpacked to `C:\Program Files\Saxonica\SaxonCHE-windows-x86_64-13-0-0`,
you'd need to add the following items to the "Path" entry:

    C:\Program Files\Saxonica\SaxonCHE-windows-x86_64-13-0-0\SaxonCHE\bin
    C:\Program Files\Saxonica\SaxonCHE-windows-x86_64-13-0-0\SaxonCCoreHE\bin

### Using as a library in your own project
If you're intending to develop against the C++ API, we do not  currently
recommend using the C++ API with the pre-compiled `libsaxonc-he`, in case of ABI
conflict. You should compile `libsaxonc-he` yourself, and only use the
pre-compiled `libsaxonc-core-he`. If you aren't planning to use the
Transform and Query binaries then you can remove the
entry for `C:\Program Files\Saxonica\SaxonCHE-windows-x86_64-13-0-0\SaxonCHE\bin` from your PATH, which will make it
less likely that your project will find itself loading the wrong DLL.

If you're intending to develop against the C API, then you can use the pre-compiled `libsaxonc-he` as well as `libsaxonc-core-he`, and you can make use of both PATH entries from the previous step.

In either case, you need to ensure that your build system can locate the header files, from `SaxonCCoreHE\include` and (if using the pre-compiled `libsaxonc-he`) from `SaxonCHE\include`.

## Running the binaries
Once you have the binaries on your path, you should be able to run the binaries
directly, for example:

	Transform -s:input.xml -xsl:stylesheet.xsl

See https://www.saxonica.com/documentation13/index.html#!using-xsl/commandline
for information about `Transform`, and
https://www.saxonica.com/documentation13/index.html#!using-xquery/commandline
for information about `Query`.

## Integrating with a CMake-using project

If you're planning to use both the provided `libsaxonc-core-he` and
`libsaxonc-he` `.dll` files, then the `samples`
directory provides a fully working example, which you can boil down to the
following snippet for a `CMakeLists.txt`:

    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/Saxonica/SaxonCHE-windows-x86_64-13-0-0")
    find_package(SaxonCHE 13.0.0 EXACT REQUIRED)
    target_link_libraries(YourTarget PRIVATE SaxonCHE::saxonc-he)

(Where `C:\Program Files\Saxonica` is where you installed the libraries.)

If you're planning to compile `libsaxonc-he` yourself, to use the C++
API, then you can use CMake's `FetchContent` in your `CMakeLists.txt` to
integrate it into your project:

    include(FetchContent)
    FetchContent_Declare(
        SaxonCHE
        URL "C:/Program Files/Saxonica/SaxonCHE-windows-x86_64-13-0-0"
        SOURCE_SUBDIR src
    )
    FetchContent_MakeAvailable(SaxonCHE)

That will automatically use the `SaxonCCoreHE` bundled as
part of the `.zip` (which we assumed you unzipped to
`C:\Program Files\Saxonica` in the example). If you need to use a different
`SaxonCCoreHE`, for example, if you're cross-compiling, then you
need to specify where CMake can find that version, and prevent it from using
the bundled version. To do that, you can set the `NO_BUNDLED_SAXONC_CORE` CMake
variable to `TRUE` and then you have to manually make a version of
`SaxonCCoreHE` available to CMake.

Here's a minimal example that assumes you have already made a `SaxonCCoreHE`
package visible to CMake:

    # We have a non-standard libsaxonc-core-he on CMake's search path,
    # but are using the C/C++ sources from the release version

    include(FetchContent)
    FetchContent_Declare(
        SaxonCHE
        URL "C:/Program Files/Saxonica/SaxonCHE-windows-x86_64-13-0-0"
        SOURCE_SUBDIR src
    )
    set(NO_BUNDLED_SAXONC_CORE TRUE)
    FetchContent_MakeAvailable(SaxonCHE)

And this is a more complete example showing cross-compilation on Linux:

    # Assume we are compiling on a Linux x86_64 host and have downloaded an Arm64
    # version of SaxonCCore to /opt/saxonica/SaxonCHE-linux-arm64-13-0-0,
    # and x86_64 version to /opt/saxonica/SaxonCHE-linux-x86_64-13-0-0
    include(FetchContent)
    FetchContent_Declare(
        SaxonCHE
        URL "/opt/saxonica/SaxonCHE-linux-x86_64-13-0-0"
        SOURCE_SUBDIR src
    )

    if(CMAKE_SYSTEM_PROCESSOR STREQUALS "aarch64")
        set(NO_BUNDLED_SAXONC_CORE TRUE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/saxonica/SaxonCHE-linux-arm64-13-0-0")
    endif()
    FetchContent_MakeAvailable(SaxonCHE)

## Other resources
SaxonC-specific samples can be found at the [saxonc-resources][] GitHub
repository.

[saxonc-resources]: https://github.com/Saxonica/saxonc-resources

Full documentation for SaxonC-HE 13 is available. The [SaxonC 13 documentation][]
contains information specific to SaxonC - including instructions for installing
and configuring the product, details of the SaxonC C/C++ API, PHP API and
Python3 API, and sample code. Meanwhile, the main [Saxon 13 documentation][]
contains further information relevant to SaxonC.

[SaxonC 13 documentation]: https://www.saxonica.com/saxon-c/documentation13/index.html
[Saxon 13 documentation]: https://www.saxonica.com/documentation13/index.html
For further information, see the [SaxonC web page][]. For bug reporting,
lists of known bugs, and help forums, please visit
[https://saxonica.plan.io/] (https://saxonica.plan.io/).

[SaxonC web page]: https://www.saxonica.com/saxon-c/index.xml
