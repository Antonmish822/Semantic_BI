<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xs="http://www.w3.org/2001/XMLSchema"
                xmlns:qtc="http://www.w3.org/2010/09/qt-fots-catalog"
                xmlns:xtc="http://www.w3.org/2012/10/xslt-test-catalog"
                xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog"
                exclude-result-prefixes="#all"
                version="3.0">

<xsl:output method="xml" encoding="utf-8" indent="yes"/>

<xsl:mode on-no-match="shallow-skip"/>

<xsl:template match="/">
  <catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog" prefer="public">
    <xsl:apply-templates/>
  </catalog>
</xsl:template>

<xsl:template match="qtc:resource|xtc:resource">
  <uri name="{@uri}" uri="{resolve-uri(@file, base-uri(.))}"/>
</xsl:template>

</xsl:stylesheet>