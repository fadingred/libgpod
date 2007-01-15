<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output 
      encoding="ASCII"
      method="text"/>

  <xsl:template match="/">
    <xsl:for-each select="//refsect2[./title/anchor/@role='function']">
      <xsl:text>%feature("docstring", "</xsl:text>
      <xsl:call-template name="CleanQuotes">
        <xsl:with-param name="body" select="para"/>
      </xsl:call-template>
      <xsl:if test="variablelist[@role='params']">
	<xsl:text>&#10;&#10;Parameters&#10;</xsl:text>
	<xsl:for-each select="variablelist[@role='params']/varlistentry">
	  <xsl:call-template name="CleanQuotes">
	    <!-- We're going to remove the nbsp chars as we're rendering to python docstrings -->
	    <xsl:with-param name="body" select="translate(term,'&#160;','')"/>
	  </xsl:call-template>
	  <xsl:call-template name="CleanQuotes">
	    <xsl:with-param name="body" select="translate(listitem,'&#160;','')"/>
	  </xsl:call-template>
	</xsl:for-each>
      </xsl:if>
      <xsl:text>") </xsl:text>
      <xsl:value-of select="substring-before(./title[./anchor/@role='function'],' ')"/>
      <xsl:text>; &#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- recursive cleaner, matches string to first quote then recurses to
       process the rest
       Note how it uses a When to test if the termination condition of no
       quotes has been reached
       -->


  <xsl:template name="CleanQuotes"><xsl:param name="body"/>
  <xsl:choose>
    <xsl:when test="contains($body, '&quot;')">
      <xsl:value-of select="substring-before($body, '&quot;')" />
        <xsl:text>\&quot;</xsl:text>
        <xsl:call-template name="CleanQuotes">
          <xsl:with-param name="body" select="substring-after($body,'&quot;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$body"/><!-- finished recursing -->
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
</xsl:stylesheet>
