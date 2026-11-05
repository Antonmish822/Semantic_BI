# SaxonC-HE 13.0.0 PHP extension

The PHP extension is in the form of a C/C++ PHP8 extension to SaxonC-HE created
using the Zend module API.

SaxonC-HE provides processing in XSLT 3.0, XQuery 3.1 and XPath 3.1, and Schema
validation 1.0/1.1.

This is a library with APIs for PHP. The main classes in the SaxonC PHP API are
as follows: SaxonProcessor, Xslt30Processor, XsltExecutable, XQueryProcessor,
XPathProcessor, SchemaValidator, and DocumentBuilder.

The following classes provide a subset of the XDM model: XdmValue,
XdmItem, XdmAtomicValue, XdmNode, XdmFunctionItem, XdmArray, and
XdmMap.

Full documentation for SaxonC-HE 13.0.0 PHP is available. The [SaxonC-HE 13documentation](https://www.saxonica.com/saxon-c/documentation13/index.html#!api/saxon_c_php_api)
contains information specific to SaxonC - including instructions for
installing and configuring the product, details of the SaxonC C/C++
API, PHP API and Python3 API, and sample code. Meanwhile, the main
[Saxon 13 documentation](https://www.saxonica.com/documentation13/index.html)
contains further information relevant to SaxonC-HE.

For further information, see the [SaxonC web
page](https://www.saxonica.com/saxon-c/index.xml). For bug reporting,
lists of known bugs, and help forums, please visit the SaxonC bug issue project:
[https://saxonica.plan.io/projects/saxon-c](https://saxonica.plan.io/projects/saxon-c).

# Installation

Users need to first install the SaxonC-HE libraries in a suitable location. (See
the main SaxonC-HE README for more details.)

## The `./configure` script

To build and install the PHP extension, open a shell window and navigate to the
`php/src` directory inside the location where you unpacked the zip file. Run
these commands:

    phpize

There are a couple of ways of telling the configure script about the location of
the SaxonC libraries, depending on how you installed them.

### Installed `libsaxonc-he` and `libsaxonc-core-he` in the same place

If you install all the binaries and libraries for `libsaxonc-he` and
`libsaxonc-core-he` in the same place (i.e. `/usr/local`), then the `lib` dir
(`/usr/local/lib` in this example) will contain both `libsaxonc-core-he.dll`
and `libsaxonc-he.dll`, and all the include files will be located under
`/usr/local/include`. You can run the `./configure` script as follows, and it
will search under a series of standard directories in order: `/opt/saxonica`,
`/usr/local`, and then `/usr`:

    ./configure --with-saxonc

You can specify the path explicitly with:

    ./configure --with-saxonc={MY-PATH}

It will search for libraries under `{MY-PATH}/lib` and headers under
`{MY-PATH}/include`.

### Installed `libsaxonc-he` and `libsaxonc-core-he` in separate folders

If you've installed `libsaxonc-he` and `libsaxonc-core-he` in separate folders,
as they come in the download `.zip`, or perhaps you have compiled `libsaxonc-he`
yourself, and are using it together with the supplied `libsaxonc-core-he`, then
you can pass the location for each library to `./configure` separately:

    ./configure --with-saxonc=/usr/local --with-saxonc-core=/opt/my-saxonc

## Installing

Once you have successfully run `./configure`, then you can run `make` as with
any other extension:

    make
    sudo make install

## Configuring PHP and enabling the extension

**Important:** You need to make sure that the paths to the directories containing
`libsaxonc-he.dll` and `libsaxonc-core-he.dll` are in the `$`
of the PHP process, or it won't be able to find them.

Create a module conf file:

    nano /etc/php/8.X/mods-available/20-saxon.ini

with content:

    ; configuration for php Saxon HE/PE/EE module
    extension=saxon.so

and save the file. Now enable the module for PHP using the command:

    phpenmod saxon

Alternatively, you can update the `php.ini` file or add the `20-saxon.ini` file
under the `conf.d` directory (if using Ubuntu it is usually in the location
`/etc/php/8.X/apache2/`) to contain the PHP extension. Insert the code
`extension=saxon.so` in the Dynamic Extensions section, then run the following
commands:

    sudo service apache2 restart
    php -m | grep saxon -i

For some Linux distributions using php-fpm the file `20-saxon.ini` should be
installed within the directory `/etc/php.d`
