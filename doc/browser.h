/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file browser.h
 * @brief Description of OMA CP browser settings supported by the CPClient.
 *
 * @section browser Browser Bookmarks
 *
 * The CPClient supports the provisioning of browser bookmarks and the browser
 * start page via OMA CP.  Only a single start page can be provisioned but
 * no limits, apart from those imposed by WAP Push, are placed upon the number
 * of bookmarks.
 *
 * The CPClient only supports a single set of browser settings.  If an OMA CP
 * document contains multiple browser application characteristics, all but the
 * first characteristic are ignored.
 *
 * Finally, all connectoid references inside a browser application
 * characteristic are also ignored.  The browser will use the device's
 * connection manager to determine the best way to connect to the Internet.
 * This may be achieved via one of the connectoids provisioned in the CP
 * document but the CPClient does not allow the document to explicitly
 * specify this.
 *
 * <table>
 * <tr><th>OMA CP Parameter</th><th>Occurrence</th><th>Description</th>
 * <th>Permissible Values</th></tr>
 * <tr><td colspan="4" align="center"><i>Supported Browser Parameters</i></td>
 * </tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as a group of Browser settings.</td><td>w2</td></tr>
 * <tr><td>APPLICATION/RESOURCE</td><td>1 or more</td>
 * <td>Characteristic that contains settings for a single bookmark</td>
 * <td></td></tr>
 * <tr><td>APPLICATION/RESOURCE/URI</td><td>1</td><td>The bookmark's URL</td>
 * <td>A URL</td></tr>
 * <tr><td>APPLICATION/RESOURCE/NAME</td><td>0 or 1</td><td>A friendly name
 * for the bookmark.</td><td>A string</td></tr>
 * <tr><td>APPLICATION/RESOURCE/STARTPAGE</td><td>0 or 1</td>
 * <td>Indicates that the bookmark should also be the start page.  Only one
 * bookmark should contain this parameter</td><td>N/A</td></tr>
 * </table>
 *
 * As an example, consider the following OMA CP document.
\code
<wap-provisioningdoc version="1.0">

  <characteristic type="APPLICATION">
    <parm name="APPID" value="w2"/>
    <characteristic type="RESOURCE">
      <parm name="URI" value="http://www.intel.com"/>
      <parm name="NAME" value="Intel"/>
      <parm name="STARTPAGE"/>
    </characteristic>
    <characteristic type="RESOURCE">
      <parm name="NAME" value="Open Source at Intel"/>
      <parm name="URI" value="http://software.intel.com/sites/oss"/>
    </characteristic>
  </characteristic>

</wap-provisioningdoc>
\endcode

It is converted by the CPClient into the six Provman keys below.

\code
/applications/browser/bookmarks/Intel/name = Intel
/applications/browser/bookmarks/Intel/url = http://www.intel.com
/applications/browser/bookmarks/Open?Source?at?Intel/name = Open Source at Intel
/applications/browser/bookmarks/Open?Source?at?Intel/url =
    http://software.intel.com/sites/oss
/applications/browser/startpage/name = Intel
/applications/browser/startpage/url = http://www.intel.com
\endcode
 *
 */
