<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <root>
      <b>
        <xsl:value-of select="document/a"/>
      </b>
    </root>
  </xsl:template>
</xsl:stylesheet>