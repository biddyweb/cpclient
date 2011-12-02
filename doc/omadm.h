/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file omadm.h
 * @brief Description of OMA CP OMA DM settings supported by
 * the CPClient.
 *
 * @section omadm OMA DM Accounts
 *
 * The CPClient allows multiple OMA DM accounts to be provisioned.
 *
 * The only restriction on the provisioning of OMA DM accounts is that it is not
 * possible to specify that a given connection or proxy should be used to access
 * the OMA DM server.  All connectoid references within an OMA DM application
 * characteristic are ignored.
 *
 * Each OMA DM characteristic defines a separate OMA DM account.  The supported
 * settings are specified in the table below.
 *
 * <table>
 * <tr><th>OMA CP Parameter</th><th>Occurrence</th><th>Description</th>
 * <th>Permissible Values</th></tr>
 * <tr><td colspan="4" align="center"><i>Supported OMA DM
 * Parameters</i></td>
 * </tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as an OMA DM account</td><td>w7</td></tr>
 * <tr><td>APPLICATION/NAME</td><td>0 or 1</td><td>Contains the user readable
 * name of OMA DM server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/PROVIDER-ID</td><td>1</td><td>ID of the OMA DM
 * Server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/INIT</td><td>0 or 1</td><td>Indicates that the
 * CPClient should automatically initiate a session with the DM server
 * being provisioned</td><td>N/A</td></tr>
 * <tr><td>APPLICATION/ADDR</td><td>0 or 1</td><td>Contains the address of the
 * OMA DM server. This parameter is used if the client should connect
 * to the server using the default port number</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR</td><td>0 or 1</td><td>Characteristic for
 * specifying the server address</td><td></td></tr>
 * <tr><td>APPLICATION/APPADDR/ADDR</td><td>1</td><td>Contains the address of
 * the OMA DM server. This parameter should be used when explicitly
 * specifying the port number. A w7 APPLICATION characteristic should not
 * include both APPLICATION/ADDR and APPLICATION/APPADDR/ADDR</td>
 * <td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR/PORT</td><td>0 or 1</td><td>The port number
 * of the OMA DM server</td><td>An integer</td></tr>
 * <tr><td>APPLICATION/APPAUTH</td><td>0 or more</td><td>Characteristic for
 * specifying the login credentials</td><td></td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHLEVEL</td><td>0 or 1</td><td>Determines what
 * the credentials in the characteristic should be used for</td><td>
 * APPSRV (The credentials the client should use when authenticating
 * with the server) or CLIENT (The credentials the server should use when
 * authenticating with the client).  Note that if this parameter is omitted,
 * then the APPAUTH characteristic contains client authentication details for
 * HTTP.</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHTYPE</td><td>1</td><td>Identifies
 * the type of authentication being used</td><td>
 * HTTP-BASIC, HTTP-DIGEST, BASIC, DIGEST, X509, SECUREID, SAFEWORD,
 * DIGIPASS</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHNAME</td><td>0 or 1</td><td>
 * The user name</td><td> A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHSECRET</td><td>0 or 1</td><td>
 * The password</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHDATA</td><td>0 or 1</td><td>
 * The nonce</td><td>A base 64 encoded binary value</td></tr>
 * </table>
 *
 * Note that OMA DM APPLICATION characteristics must contain a server
 * address. If a server address is not supplied by APPLICATION/ADRR or
 * APPLICATION/APPADDR/ADDR, the characteristic is deemed to be invalid.
 *
 * An example of an OMA CP document that provisions an OMA DM account is
 * shown below.
\code
<?xml version="1.0"?>
<!DOCTYPE wap-provisioningdoc PUBLIC "-//WAPFORUM//DTD PROV 1.0//EN"
"http://www.wapforum.org/DTD/prov.dtd">
<wap-provisioningdoc version="1.0">

  <characteristic type="APPLICATION">
    <parm name="APPID" value="w7"/>
    <parm name="PROVIDER-ID" value="dmserver"/>
    <parm name="NAME" value="DM Server"/>

    <characteristic type="APPADDR">
      <parm name="ADDR" value="http://127.0.0.1/dm"/>
      <characteristic type="PORT">
        <parm name="PORTNBR" value="8080"/>
      </characteristic>
    </characteristic>

    <characteristic type="APPAUTH">
      <parm name="AAUTHTYPE" value="DIGEST"/>
      <parm name="AAUTHLEVEL" value="APPSRV"/>
      <parm name="AAUTHNAME" value="username"/>
      <parm name="AAUTHSECRET" value="password"/>
    </characteristic>

    <characteristic type="APPAUTH">
      <parm name="AAUTHTYPE" value="DIGEST"/>
      <parm name="AAUTHLEVEL" value="CLIENT"/>
      <parm name="AAUTHNAME" value="username"/>
      <parm name="AAUTHSECRET" value="password"/>
    </characteristic>
  </characteristic>

</wap-provisioningdoc>
\endcode
 *
 * It is converted into the following Provman keys by the CPClient.
\code
/applications/omadm/dmserver/name = DM Server
/applications/omadm/dmserver/server_id = dmserver
/applications/omadm/dmserver/url = http://127.0.0.1/dm:8080
/applications/omadm/dmserver/client_creds/authtype = digest
/applications/omadm/dmserver/client_creds/password = password
/applications/omadm/dmserver/client_creds/username = username
/applications/omadm/dmserver/server_creds/authtype = digest
/applications/omadm/dmserver/server_creds/password = password
/applications/omadm/dmserver/server_creds/username = username
\endcode
 *
 * Provman allows device management clients to associate meta data with
 * its settings.  The CPClient only does this in one instance;
 * when it processes an OMA CP document that contains one or more valid
 * OMA DM accounts.  In such cases, the CPClient generates one meta
 * data key for each account, each group of connection settings (including MMS)
 * and for browser settings provisioned in the same document.  No meta
 * data keys are generated for existing accounts, created either by the user
 * or provisioned in other documents.  The meta data key is called "ACL"
 * and is associated with the root key of accounts and connection
 * settings, or the root key of the browser settings. e.g.,
 * /applications/omadm/dmserver and /telephony/contexts/3gcontext.  The value
 * of the ACL meta data property is OMA DM specific.  It grants
 * all the DM servers provisioned in the OMA CP document rights to modify any
 * of the accounts, telephony settings or browser bookmarks provisioned in the
 * document.  For example, the following meta data property is created by the
 * CPClient for the OMA CP document presented above.  Note this restriction is
 * not imposed by Provman.  It needs to be enforced by an OMA DM client that
 * uses provman to provision settings.
 *
 * <table>
 * <tr><th>Key</th><th>Meta data Property Name</th><th>Value</th></tr>
 * <tr><td>/applications/omadm/dmserver</td><td>ACL</td>
 * <td>Add=dmserver&Delete=dmserver&Replace=dmserver&Get=dmserver</td></tr>
 * </table>
 *
 * Finally, when the CPClient processes an OMA CP document, it maintains a list
 * of all the DM accounts provisioned in that document that contain the INIT
 * parameter.  This list is made available via the d-Bus API to the platform
 * specific CPClient, i.e., the process that implements the UI and registers
 * to receive the OMA CP WAP Push messages.  This platform specific client
 * should initiate a session with all the DM servers whose ids appear in this
 * list, as soon as the message has been successfully provisioned.  Obviously,
 * it can only do this if the device contains a DM Client.
 */
