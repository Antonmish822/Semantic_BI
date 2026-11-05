////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 - 2025 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/. This Source Code Form is
// "Incompatible With Secondary Licenses", as defined by the Mozilla Public
// License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef XSDSCHEMA_H
#define XSDSCHEMA_H

#include "saxonc_export.h"
#include "saxonc/SaxonProcessor.h"

class XsdmCompiler;

/*!
 * This class represents an XSD Schema - that is, a set of schema components, typically resulting
 * from compiling a collection of one or more source schema documents.
 *
 * <p>The class exists in all Saxon editions as an abstract place-holder class, but concrete
 * implementations exist only in SaxonC-EE.</p>
 */

class SAXONC_EXPORT XsdSchema {


public:

    /**
     * Destructor method to clean up the XsdScherma
     */
    ~XsdSchema();

    SchemaValidator * newValidator();


    /**
     * Combine this schema with another, to form a new combined schema.
     * <p>Note: in relatively unusual circumstances, a document that is valid against one of the two input schemas
     * might not be valid against the combined schema. For example, if the first schema has a wildcard with
     * <code>processContents="lax"</code> then an element might be laxly validated when assessed using that schema,
     * but strictly validated when assessed against the combined schema; and strict validation may fail while
     * lax validation succeeded.</p>
     * @param other the other schema to combine with this one.
     * @return the combined schema, assuming the two schemas are compatible
     * @throws SaxonApiException if the two schemas cannot be combined, for example because they
     * contain components with duplicated names
     * @since 13.0
     */

    XsdSchema * combine(XsdSchema * other);

    /**
     * Get the underlying implementation-level Schema
     */

    int64_t getUnderlyingSchema();

    /**
     * Export a precompiled Schema Component Model containing all the components (except built-in components)
     * in this schema.
     *
     * @param destination the destination to recieve the precompiled Schema Component Model in the form of an
     *                    XML document
     * @throws SaxonApiException if a failure occurs writing the schema components to the supplied destination
     */

    void exportComponents(const char * filename);




private:

    /**
     * Default constructor.
     * Creates a Schema Validator
     * @throws SaxonApiException
     */
    XsdSchema();

    /**
     * Constructor with the SaxonProcessor supplied.
     * The supplied processor should have license flag set to true for the Schema
     * Validator to operate.
     * @param cwd - the current working directory
     * @throws SaxonApiException
     */
    XsdSchema(std::string cwd, int64_t procRef, int64_t exObject);

    /**
     * XsdSchema copy constructor.
     * @param other - XsdScherma
     */
    XsdSchema(const XsdSchema &other);

    friend class SchemaValidator;
    friend class XsdCompiler;
    std::string cwdS; /*!< Current working directory */
    int64_t xsdSchemaObj;     /*!< The underlying SchemaCompiler Java object */
    int64_t procRef;
};


#endif //XSDSCHEMA_H
