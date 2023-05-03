<?xml version="1.0" ?>

<!-- FILE   : XEmail.xsl
     AUTHOR : Peter C. Chapin <PChapin@vtc.vsc.edu>
     SUBJECT: Style sheet to convert XEmail instance documents into HTML.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:email="http://www.vtc.edu/XML/XEmail" xmlns:xhtml="http://www.w3.org/1999/xhtml">

  <xsl:output method="html"/>

  <!-- The following template defines the high level structure of the resulting document. -->
  <xsl:template match="email:message-group">
    <html>
      <head>
        <title>Email Archive</title>
      </head>
      <body>
        <h1>Email Archive</h1>
        <hr/>
        <xsl:apply-templates select="email:message"/>
        <p>Produced by XEmail Styler (blah, blah, blah)</p>
      </body>
    </html>
  </xsl:template>

  <!-- The following template describes the format of an individual message. -->
  <xsl:template match="email:message">
    <xsl:apply-templates select="header"/>
    <div>
      <xsl:value-of select="body/xhtml:html/xhtml:body"/>
    </div>
    <hr/>
  </xsl:template>

  <!-- The header has its own template. It's nice to encapsulate header formatting in one place. -->
  <xsl:template match="header">
    <table border="0">
      <tbody>
        <tr>
          <td valign="top">
            <b>From</b>
          </td>
          <td>:</td>
          <td>
            <xsl:apply-templates select="From/address"/>
          </td>
        </tr>
        <tr>
          <td valign="top">
            <b>To</b>
          </td>
          <td>:</td>
          <td>
            <xsl:apply-templates select="To/address"/>
          </td>
        </tr>
        <tr>
          <td valign="top">
            <b>Subject</b>
          </td>
          <td>:</td>
          <td>
            <xsl:value-of select="Subject"/>
          </td>
        </tr>
        <tr>
          <td valign="top">
            <b>Date</b>
          </td>
          <td>:</td>
          <td><xsl:value-of select="translate(substring(Date, 1, 10), '-', '/')"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="substring(Date, 12, 2)"/>h<xsl:value-of
              select="substring(Date, 15, 2)"/>m </td>
        </tr>
      </tbody>
    </table>
  </xsl:template>

  <!-- This is how an address in an address list is formatted. -->
  <xsl:template match="address">
    <xsl:choose>
      <xsl:when test="full-name">
        <xsl:value-of select="full-name"/> &lt;<a>
          <xsl:attribute name="href">mailto:<xsl:value-of select="email"/></xsl:attribute>
          <xsl:value-of select="email"/>
        </a>&gt; </xsl:when>
      <xsl:otherwise>
        <a>
          <xsl:attribute name="href">mailto:<xsl:value-of select="email"/></xsl:attribute>
          <xsl:value-of select="email"/>
        </a>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="position() != last()">
      <xsl:text>, </xsl:text>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
