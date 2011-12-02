/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file synchronisation.h
 * @brief Description of OMA CP data synchronisation settings supported by
 * the CPClient.
 *
 * @section synchronisation Data Synchronisation Accounts
 *
 * The CPClient allows multiple data synchronisation (OMA DS) accounts to be
 * provisioned.
 *
 * The only restriction on the provisioning of OMA DS accounts is that it is not
 * possible to specify that a given connection or proxy should be used to access
 * the OMA DS server.  All connectoid references within an OMA DS application
 * characteristic are ignored.
 *
 * Each OMA DS characteristic defines a separate OMA DS account.  The supported
 * settings are specified in the table below.
 *
 * <table>
 * <tr><th>OMA CP Parameter</th><th>Occurrence</th><th>Description</th>
 * <th>Permissible Values</th></tr>
 * <tr><td colspan="4" align="center"><i>Supported Data Synchronisation
 * Parameters</i></td>
 * </tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as an OMA DS account</td><td>w5</td></tr>
 * <tr><td>APPLICATION/NAME</td><td>0 or 1</td><td>Contains the user readable
 * name of OMA DS server.</td><td>A string</td></tr>
 * <tr><td>APPLICATION/PROVIDER-ID</td><td>0 or 1</td><td>ID of the OMA DS
 * Server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/ADDR</td><td>0 or 1</td><td>Contains the address of the
 * OMA DS server. This parameter is used if the client should connect
 * to the server using the default port number.</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR</td><td>0 or 1</td><td>Characteristic for
 * specifying the server address.</td><td></td></tr>
 * <tr><td>APPLICATION/APPADDR/ADDR</td><td>1</td><td>Contains the address of
 * the OMA DS server. This parameter should be used when explicitly
 * specifying the port number. A w5 APPLICATION characteristic should not
 * include both APPLICATION/ADDR and APPLICATION/APPADDR/ADDR. </td>
 * <td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR/PORT</td><td>0 or 1</td><td>The port number
 * of the OMA DS server</td><td>An integer</td></tr>
 * <tr><td>APPLICATION/APPAUTH</td><td>0 or 1</td><td>Characteristic for
 * specifying the server's login credentials</td><td></td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHLEVEL</td><td>1</td><td>Determines what
 * the credentials in the characteristic should be used for</td><td>
 * APPSRV (This is the only value supported)</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHTYPE</td><td>1</td><td>Identifies
 * the type of authentication being used</td><td>
 * HTTP-BASIC, HTTP-DIGEST, BASIC, DIGEST, X509, SECUREID, SAFEWORD,
 * DIGIPASS</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHNAME</td><td>0 or 1</td><td>
 * The user name</td><td> A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHSECRET</td><td>0 or 1</td><td>
 * The password</td><td>A string</td></tr>
 * <tr><td>APPLICATION/RESOURCE</td><td>1 or more</td><td>
 * Characteristic that identifies a server resource to be synchronised</td>
 * <td></td></tr>
 * <tr><td>APPLICATION/RESOURCE/AACCEPT</td><td>1</td><td>
 * Specifies the content types accepted by the server</td>
 * <td> It is a comma separated list of MIME Types</td></tr>
 * <tr><td>APPLICATION/RESOURCE/URI</td><td>1</td><td>
 * URI of the server database</td><td> A URI</td></tr>
 * <tr><td>APPLICATION/RESOURCE/NAME</td><td>0 or 1</td><td>
 * User readable name of the server database</td><td>contacts, calendar,
 * todo or memo.  Other names may be provided but the cpclient may choose
 * to override the user supplied names if it can determine that one of the
 * standard names would be more suitable by looking at the AACCEPT parameter
 * </td></tr><tr><td>APPLICATION/RESOURCE/SYNCTYPE</td><td>0 or 1</td><td>
 * Specifies the type of synchronisation to perform</td>
 * <td>An integer.  The following values are permitted:
 * 1 (Slow Synch), 2 (Two-way Sync), 3 (One-way Sync), 4 (Refresh Sync from
 * client), 5 (One way sync from server), 6 (Refresh Sync from Server).
 * </td></tr>
 * </table>
 *
 * Note that OMA DS APPLICATION characteristics must contain a server
 * address. If a server address is not supplied by APPLICATION/ADRR or
 * APPLICATION/APPADDR/ADDR, the characteristic is deemed to be invalid.
 * In addition, each APPLICATION characteristic that provisions an OMA DS
 * application must contain at least one RESOURCE characteristic.
 *
 * An example of a OMA CP document that provisions an OMA data synchronisation
 * account is shown below.
\code
<wap-provisioningdoc version="1.0">

  <characteristic type="APPLICATION">
    <parm name="APPID" value="w5"/>
    <parm name="PROVIDER-ID" value="dslocal"/>
    <parm name="NAME" value="Data Sync Account"/>
    <parm name="ADDR" value="http://127.0.0.1/syncme"/>
    <characteristic type="APPAUTH">
      <parm name="AAUTHLEVEL" value="APPSRV"/>
      <parm name="AAUTHTYPE" value="BASIC"/>
      <parm name="AAUTHNAME" value="username"/>
      <parm name="AAUTHSECRET" value="password"/>
    </characteristic>
    <characteristic type="RESOURCE">
      <parm name="URI" value="./addressbook/myaddresses"/>
      <parm name="AACCEPT" value="text/x-vcard"/>
      <parm name="SYNCTYPE" value="1"/>
    </characteristic>
  </characteristic>

</wap-provisioningdoc>
\endcode
 *
 * It is converted into the following Provman keys by the CPClient.
\code
/applications/sync/dslocal/name = Data Sync Account
/applications/sync/dslocal/url = http://127.0.0.1/syncme
/applications/sync/dslocal/client = 0
/applications/sync/dslocal/username = username
/applications/sync/dslocal/password = password
/applications/sync/dslocal/contacts/uri = ./addressbook/myaddresses
/applications/sync/dslocal/contacts/sync = slow
/applications/sync/dslocal/contacts/format = text/x-vcard
\endcode
 *
 * Provman defines four standard sub-directories for remote databases.
 * These are "contacts", "calendar", "todo", "memo".  The CPClient
 * uses the RESOURCE/NAME to determine which sub-directory to use.  If
 * the RESOURCE/NAME parameter is not specified or it does not match
 * one of the standard sub-directory names, mentioned above, the CPClient
 * will attempt to determine the appropriate sub-directory from the resources'
 * AACCEPT parameter.  In the example above, contacts was chosen, as no
 * name was explicitly specified and the AACEPT parameter contained a
 * x-vcard MIME type.
 */
