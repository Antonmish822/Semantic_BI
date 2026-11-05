////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 - 2025 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/. This Source Code Form is
// "Incompatible With Secondary Licenses", as defined by the Mozilla Public
// License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef QNAME_H
#define QNAME_H

#include "saxonc_export.h"
#include "saxonc/SaxonProcessor.h"



class SAXONC_EXPORT QName {
public:

    QName(int64_t obj);

    QName(const char * prefix, const char * uri, const char * localName);

    QName(const char * uri, const char * lexical);

    QName(const char * localName);

    QName(const char * lexicalQName, XdmNode * element);

    QName(const QName &other);

    ~QName();

    bool operator==(const QName &other) const;

    static QName * fromClarkName(const char * expandedName);

    static QName * fromEQName(const char * expandedName);

    bool isValid();

    const char * getPrefix();

    const char * getNamespace();

    const char * getNamespaceUri();

    const char * getLocalName();

    const char * getClarkName();

    const char * getEQName();

    const char * toString();

    int64_t getUnderlyingValue();





private:
    int64_t qnameObj;     /*!< The underlying SchemaCompiler Java object */
}

#endif //QNAME_H
