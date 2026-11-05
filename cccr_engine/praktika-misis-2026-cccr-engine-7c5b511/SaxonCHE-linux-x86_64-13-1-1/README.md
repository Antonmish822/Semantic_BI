# SaxonC-HE 13.1.1 download:
SaxonC-HE is a compiled variant of Saxon from the Java
platform to the C/C++ platform using GraalVM `native-image`.

SaxonC-HE provides processing in XSLT 3.0, XQuery 3.1 and XPath 3.1, and,
in SaxonC-EE, Schema validation 1.0/1.1.

## What's included in this download?
In this download, the binary shared libraries (.so files) for the
core part of Saxon, compiled with `native-image`, which we call
`libsaxonc-core-he`, along with the sources for the C/C++-specific API,
which we call `libsaxonc-he`, and its binary shared libraries, along with
the Transform and Query binaries for running XSLT and XQuery from the
command line.

## Installation
If you're intending to use the pre-compiled binaries, like Transform and Query,
then installation on your system is a matter of copying the libraries, and 
binaries to your preferred install location, ensuring the libraries can be found 
at runtime, and the binaries are on your PATH.

If you're intending to also use the C API, then you need to ensure the header 
files are also installed somewhere your build system can find them. We do not
currently recommend using the C++ API with the pre-compiled `libsaxonc-he` in
case of ABI conflict, for that you can compile `libsaxonc-he` yourself.

If you're intending to compile `libsaxonc-he` yourself, then what you need to do
will depend on your project and its build system. We provide out-of-the-box 
compatibility with CMake, but using other systems should be straightforward.

### Installing the files
We recommend unpacking the release `.zip` under
`/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1`, and then adding the relevant
folders to your PATH and LD_LIBRARY_PATH  environment variables. This allows
you to install more than one version, or edition, side-by-side,
should you need it.

For example:

	mkdir -p /opt/saxonica
    cd /opt/saxonica
    unzip /path/to/SaxonCHE-linux-x86_64-13-1-1.zip

### Set the LD_LIBRARY_PATH  and PATH environment variables
The LD_LIBRARY_PATH  variable must include the locations of the lib
directories containing the SaxonC libraries. For instance:

	export LD_LIBRARY_PATH ="/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1/SaxonCCoreHE/lib:/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1/SaxonCHE/lib:$LD_LIBRARY_PATH "

If you're only going to use the pre-compiled `libsaxonc-core-he`, and compile
`libsaxonc-he` yourself, you only need to add the `SaxonCCoreHE` lib path to
`LD_LIBRARY_PATH `.

To run the  binaries without referencing their full
path, the PATH variable can be set. For instance:

	export PATH="/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1/SaxonCHE/bin:$PATH"

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
`libsaxonc-he` `.so` files, then the `samples`
directory provides a fully working example, which you can boil down to the
following snippet for a `CMakeLists.txt`:

    list(APPEND CMAKE_PREFIX_PATH "/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1")
    find_package(SaxonCHE 13.1.1 EXACT REQUIRED)
    target_link_libraries(YourTarget PRIVATE SaxonCHE::saxonc-he)

(Where `/opt/saxonica` is where you installed the libraries.)

If you're planning to compile `libsaxonc-he` yourself, to use the C++
API, then you can use CMake's `FetchContent` in your `CMakeLists.txt` to
integrate it into your project:

    include(FetchContent)
    FetchContent_Declare(
        SaxonCHE
        URL "/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1"
        SOURCE_SUBDIR src
    )
    FetchContent_MakeAvailable(SaxonCHE)

That will automatically use the `SaxonCCoreHE` bundled as
part of the `.zip` (which we assumed you unzipped to
`/opt/saxonica` in the example). If you need to use a different
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
        URL "/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1"
        SOURCE_SUBDIR src
    )
    set(NO_BUNDLED_SAXONC_CORE TRUE)
    FetchContent_MakeAvailable(SaxonCHE)

And this is a more complete example showing cross-compilation on Linux:

    # Assume we are compiling on a Linux x86_64 host and have downloaded an Arm64
    # version of SaxonCCore to /opt/saxonica/SaxonCHE-linux-arm64-13-1-1,
    # and x86_64 version to /opt/saxonica/SaxonCHE-linux-x86_64-13-1-1
    include(FetchContent)
    FetchContent_Declare(
        SaxonCHE
        URL "/opt/saxonica/SaxonCHE-linux-x86_64-13-1-1"
        SOURCE_SUBDIR src
    )

    if(CMAKE_SYSTEM_PROCESSOR STREQUALS "aarch64")
        set(NO_BUNDLED_SAXONC_CORE TRUE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/saxonica/SaxonCHE-linux-arm64-13-1-1")
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
