////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 - 2025 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/. This Source Code Form is
// "Incompatible With Secondary Licenses", as defined by the Mozilla Public
// License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef XSDCOMPILER_H
#define XSDCOMPILER_H

#include "saxonc_export.h"
#include "saxonc/SaxonProcessor.h"
#include "saxonc/XsdSchema.h"

#include <string>
#include <vector>

class SaxonProcessor;
class XdmValue;
class XdmNode;
class XdmItem;
class XsdSchema;

/*!
* The <code>XsdCompiler</code> is used to load schema documents, and to set options for the way in
 * which they are loaded.
 *
 * <p><i>The <code>XsdCompiler</code> is new in Saxon 13; it replaces the internal class <code>SchemaManager</code>
 * which is retained (but deprecated) to provide a degree of backwards compatibility.</i></p>
 *
 * <p>A schema, represented by an <code>XsdSchema</code> object, is a collection of schema components,
 * which may come from one schema document or from a collection of schema documents.
 * An <code>XsdCompiler</code> is used to create {@link XsdSchema} objects from source
 * schema documents.</p>
 *
 * <p>Schema processing requires Saxon-EE.
 *
 * <p>An <code>XsdCompiler</code> is created using the factory method <code>SaxonProcessor.newXsdCompiler</code>. The normal pattern
 * of usage is to set options, then call <code>compileFromFile(onst char * file...)</code> or <code>compileFromString(const char* content...)</code> to compile
 * one or more schema documents into a schema. The schema can be used to validate instance documents using
 * the method <code>XsdSchema.newValidator()</code>, and it can be added to the static context of XQuery queries
 * and XSLT stylesheets using the methods <code>XQueryCompiler#useSchema(XsdSchema)</code> and
 * <code>XsltCompiler#useSchema(XsdSchema)</code>.</p>
 *
 * @since 13.0
 */
class SAXONC_EXPORT XsdCompiler {

public:

    /**
     * Default constructor.
     * Creates a Schema Validator
     * @throws SaxonApiException
     */
    XsdCompiler();

    /**
     * Constructor with the SaxonProcessor supplied.
     * The supplied processor should have license flag set to true for the Schema
     * Validator to operate.
     * @param proc - pointer to the SaxonProcessor object
     * @param cwd - the current working directory
     * @throws SaxonApiException
     */
    XsdCompiler(SaxonProcessor *proc, std::string cwd = "");

    /**
     * XsdCompiler copy constructor.
     * @param other - XsdCompiler
     */
    XsdCompiler(const XsdCompiler &other);

    /**
     * Destructor method to clean up the XsdCompiler
     */
    ~XsdCompiler();


    /**
    * The copy assignment= operator.
    * Create a copy of the XsdCompiler.
    * @param other - XsdCompiler object
    */
    //XsdCompiler &operator=(const XsdCompiler &other);

 /**
  * Set the current working directory for the validator.
  * @param cwd  - Supplied working directory which replaces any set cwd. Ignore
  * if cwd is NULL.
  */
 void setcwd(const char *cwd);

 /**
  * Get the current working directory set on this validator.
  * Memory deallocation is handled internally.
  * @return Current working directory. Memory deallocation is handled internally.
  */
 const char * getcwd();


    /**
     * Set the version of XSD in use for this schema. The value must be "1.0" or "1.1". The default is taken
     * from the configuration property {@link Feature#XSD_VERSION} at the time the
     * {@code XsdSchemaCompiler} is instantiated. By default this is "1.1".
     *
     * @param version the version of the XSD specification/language: either "1.0" or "1.1".
     */

    void setXsdVersion(const char* version);

    /**
     * Get the version of XSD in use for this schema. The value will be "1.0" or "1.1"
     *
     * @return the version of XSD in use.
     */

    const char * getXsdVersion();

    /**
     * Set the ErrorReporter to be used while loading and validating schema documents
     *
     * @param reporter The error reporter to be used. This is notified of all errors detected during the
     *                 compilation. May be set to null to revert to the default ErrorReporter.
     */
    //TODO maybe some list object void setErrorReporter(ErrorReporter reporter);

    /**
     * Get the ErrorReporter being used while loading and validating schema documents
     *
     * @return reporter The error reporter in use. This is notified of all errors detected during the
     * compilation. Returns null if no user-supplied ErrorReporter has been set.
     */

    //TODO need to report errors in SaxonC ErrorReporter getErrorReporter();



    /**
     * Load a schema document from a given Source. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an {@link XsdSchema} object.
     *
     * <p>This method can only be called once on a given {@code SchemaManager} object.</p>
     *
     * <p>If the source is a {@code StreamSource} or a {@code SAXSource} with no user-supplied {@code XmlReader},
     * then Saxon will allocate and configure an {@code XmlReader}. This will NOT take account of any configuration
     * level parser settings (such as DTD validation, XInclude processing, and so on). The recommended way to customise
     * parsing options is by creating an {@code XmlReader} and supplying this as a property of a {@code SAXSource}. For
     * options when parsing included or imported stylesheet modules, it is possible to do the same in the
     * {@link SchemaURIResolver} set using the {@link #setSchemaURIResolver(SchemaURIResolver)} method.</p>
     *
     * @param source the documents containing the schema. The getSystemId() method applied to each Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format.
    * @param encoding - the encoding of the stylesheet string. If not specified
    * then the platform default encoding is used.
     * @return the resulting schema.
     * @throws SaxonApiException if any of the schema documents is not valid, or if there are conflicts
     * between the schema documents.
     */
    XsdSchema * compileFromString(const char * source, const char *encoding = nullptr);


    /**
     * Load a schema document from a given File or set of files as a vector. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an <code>XsdSchema</code> object.
     *
     * @param filenames of the documentscontaining the schema as a vector. The getSystemId() method applied to this Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format (as produced using the -export option)
     * @return the resulting schema.
     * @throws SaxonApiException if the schema document is not valid.
     * @since 13.0
     */

    XsdSchema * compileFromFiles(const std::vector<std::string>&filenames);

    /**
     * Load a schema document from a given File or set of files as a list. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an <code>XsdSchema</code> object.
     *
     * @param filenamesList
     * @param filenames of the documentscontaining the schema as a list. The getSystemId() method applied to this Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format (as produced using the -export option)
     * @return the resulting schema.
     * @throws SaxonApiException if the schema document is not valid.
     * @since 13.0
     */
    XsdSchema * compileFromFiles(std::initializer_list<std::string> filenamesList);

    /**
     * Load a schema document from a given File or set of files as a vector. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an <code>XsdSchema</code> object.
     *
     * @param filenames of the documentscontaining the schema as an array of const char. The getSystemId() method applied to this Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format (as produced using the -export option)
     * @param count - number of items in the array
     * @return the resulting schema.
     * @throws SaxonApiException if the schema document is not valid.
     * @since 13.0
     */
    XsdSchema * compileFromFiles(const char ** filenames, int count);



    /**
     * Load a schema document from a given File or set of files. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an <code>XsdSchema</code> object.
     *
     * @param filename the document containing the schema. The getSystemId() method applied to this Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format (as produced using the -export option)
     * @return the resulting schema.
     * @throws SaxonApiException if the schema document is not valid.
     * @since 13.0
     */
    XsdSchema * compileFromFile(const char * filename);

    /**
     * Load a schema document from Node. The schema components derived from this schema
     * document are retained by this SchemaManager as the current schema, and are also returned
     * by the method as an <code>XsdSchema</code> object.
     *
     * @param node  - the node containing the schema. The getSystemId() method applied to this Source
     *               must return a base URI suitable for resolving <code>xs:include</code> and <code>xs:import</code>
     *               directives. The document may be either a schema document in source XSD format, or a compiled
     *               schema in Saxon-defined SCM format (as produced using the -export option)
     * @return the resulting schema.
     * @throws SaxonApiException if the schema document is not valid.
     * @since 13.0
     */
    XsdSchema * compileFromNode(XdmNode *node);

    /**
     * Combine a set of schemas into one
     * @param schemas the input schemas. These are not modified.
     * @return the combined schema
     * @throws SaxonApiException if the schemas are not compatible, for example if they contain
     * duplicate names.
     * @since 13.0
     */
    XsdSchema * combine(XsdSchema ** schemas, int num_schemas);

    /**
     * Return an empty schema. This may be useful for validating instance documents when all
     * the actual schema definitions are linked from the instance document using <code>xsi:schemaLocation</code>
     * and <code>xsi:noNamespaceSchemaLocation</code> attributes
     * @return an empty schema
     */
    XsdSchema * emptySchema();

    /**
     * Import a precompiled Schema Component Model from a given Source. The schema components derived from this schema
     * document are added to the cache of schema components maintained by this SchemaManager
     *
     * @param source the XML file containing the schema component model, as generated by a previous call on
     *               {@link XsdSchema#exportComponents}
     * @return the schema produced by processing the supplied schema component model export file
     * @throws SaxonApiException
     */
    XsdSchema * importComponents(const char * filename);

    /**
     * Import a precompiled Schema Component Model from a given Source. The schema components derived from this schema
     * document are added to the cache of schema components maintained by this SchemaManager
     *
     * @param source the XML file containing the schema component model, as generated by a previous call on
     *               {@link XsdSchema#exportComponents}
     * @return the schema produced by processing the supplied schema component model export file
     * @throws SaxonApiException
     */
    XsdSchema * importComponentsFromNode(XdmNode *node);

    void setProperty(const char *name, const char *value);

    /**
     * Clear configuration property values set
     */
    void clearProperties();


private:

    friend class XsdSchema;

 int64_t xsdCompilerObj;     /*!< The underlying SchemaCompiler Java object */
 int64_t procRef;     /*!< The underlying Java object for the SaxonProcessor, which belongs to the XsdCompiler */
 std::string cwdV; /*!< Current working directory */
    std::string
        xsdVersion; /*!< The version of the XSD specification/language: either "1.0" or "1.1". */
 std::map<std::string, XdmValue *>
     parameters; /*!< Map of parameters used for the schema compiler as (string,
                    value) pairs */
 std::map<std::string, std::string>
     properties; /*!< Map of properties used for the schema compiler as (string,
                    string) pairs */

};


#endif //XSDCOMPILER_H
