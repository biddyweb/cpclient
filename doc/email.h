/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file email.h
 * @brief Description of email settings supported by the CPClient.
 *
 * @section email Email Accounts
 *
 * The CPClient allows multiple email accounts to be provisioned.  Two incoming
 * protocols, POP and IMAP4 and one outgoing email protocol, SMTP, are
 * supported.  Typically, OMA CP documents always contain an even number of
 * APPLICATION characteristics that provision email applications. These
 * characteristics are grouped into sets of two by the PROVIDER-ID parameter.
 * One characteristic in each set contains the details for the server used to
 * send email (the SMTP server, for example). The other contains the details of
 * the server from which email can be retrieved (such as a POP3 or an IMAP4
 * server). So a combination of two APPLICATION characteristics with the same
 * PROVIDER-ID correspond to a single Provman email account.
 *
 * The only restriction on the provisioning of email accounts is that it is not
 * possible to specify that a given connection or proxy should be used to access
 * the various email servers.  All connectoid references within an email
 * application characteristic are ignored.
 *
 * The supported email settings are specified in the table below.
 *
 * <table>
 * <tr><th>OMA CP Parameter</th><th>Occurrence</th><th>Description</th>
 * <th>Permissible Values</th></tr>
 * <tr><td colspan="4" align="center"><i>Supported SMTP
 * Parameters</i></td></tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as one containing settings for an SMTP server</td>
 * <td>25</td></tr>
 * <tr><td>APPLICATION/NAME</td><td>0 or 1</td><td>Contains the user readable
 * name of the email account</td><td>A string</td></tr>
 * <tr><td>APPLICATION/PROVIDER-ID</td><td>0 or 1</td><td>Binds this
 * characteristic to an IMAP4 or a POP3 characteristic</td>
 * <td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR</td><td>1</td><td>Characteristic for
 * specifying the server address, service and port number</td><td></td></tr>
 * <tr><td>APPLICATION/APPADDR/ADDR</td><td>1</td><td>Contains the address of
 * the SMTP server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR/PORT</td><td>0 or 1</td><td>The port number
 * of the SMTP server</td><td>An integer</td></tr>
 * <tr><td>APPLICATION/APPADDR/SERVICE</td><td>0 or 1</td><td>Indicates whether
 * or not secure SMTP over SSL is to be used </td><td>465 indicates that
 * secure SMTP is being used, 25 indicates that it is not</td></tr>
 * <tr><td>APPLICATION/APPAUTH</td><td>0 or 1</td><td>Characteristic for
 * specifying the server's login credentials</td><td></td></tr>
* <tr><td>APPLICATION/APPAUTH/AAUTHTYPE</td><td>0 or 1</td><td>
 * Indicates the type of authentication to be used</td><td>PLAIN, NTLM,
 * GSSAPI, CRAM-MD5, DIGEST-MD5, POPB4SMTP, LOGIN</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHNAME</td><td>0 or 1</td><td>
 * The user name</td><td> A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHSECRET</td><td>0 or 1</td><td>
 * The password</td><td>A string</td></tr>
 * <tr><td>APPLICATION/FROM</td><td>1</td><td>Specifies the from email address
 * </td><td>A string</td></tr>
 * <tr><td colspan="4" align="center"><i>Supported POP3 Parameters</i></td></tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as one containing settings for a POP3 server</td>
 * <td>110</td></tr>
 * <tr><td>APPLICATION/NAME</td><td>0 or 1</td><td>Contains the user readable
 * name of the email account</td><td>A string</td></tr>
 * <tr><td>APPLICATION/PROVIDER-ID</td><td>0 or 1</td><td>Binds this
 * characteristic to an SMTP characteristic</td>
 * <td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR</td><td>1</td><td>Characteristic for
 * specifying the server address, service and port number</td><td></td></tr>
 * <tr><td>APPLICATION/APPADDR/ADDR</td><td>1</td><td>Contains the address of
 * the POP3 server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR/PORT</td><td>0 or 1</td><td>The port number
 * of the POP3 server</td><td>An integer</td></tr>
 * <tr><td>APPLICATION/APPADDR/SERVICE</td><td>0 or 1</td><td> Indicates
 * whether traffic between the client and the server should be encrypted
 * <td>995 indicates that secure POP over SSL should be used as described
 * in RFC2595.  110 indicates that an unsecured connection should be made
 * to the port number specified in the PORTNBR parameter.  If this parameter
 * is omitted, SSL is not used</td></tr>
 * <tr><td>APPLICATION/APPAUTH</td><td>0 or 1</td><td>Characteristic for
 * specifying the server's login credentials</td><td></td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHTYPE</td><td>0 or 1</td><td>
 * Indicates the type of authentication to be used</td><td>PLAIN, NTLM,
 * GSSAPI, CRAM-MD5, DIGEST-MD5, POPB4SMTP, APOP</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHNAME</td><td>0 or 1</td><td>
 * The user name</td><td> A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHSECRET</td><td>0 or 1</td><td>
 * The password</td><td>A string</td></tr>
 * <tr><td colspan="4" align="center"><i>Supported IMAP4 Parameters</i></td>
 * </tr><tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as one containing settings for a IMAP4 server</td>
 * <td>143</td></tr>
 * <tr><td>APPLICATION/NAME</td><td>0 or 1</td><td>Contains the user readable
 * name of the email account</td><td>A string</td></tr>
 * <tr><td>APPLICATION/PROVIDER-ID</td><td>0 or 1</td><td>Binds this
 * characteristic to an SMTP characteristic</td>
 * <td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR</td><td>1</td><td>Characteristic for
 * specifying the server address, service and port number</td><td></td></tr>
 * <tr><td>APPLICATION/APPADDR/ADDR</td><td>1</td><td>Contains the address of
 * the IMAP4 server</td><td>A string</td></tr>
 * <tr><td>APPLICATION/APPADDR/PORT</td><td>0 or 1</td><td>The port number
 * of the IMAP4 server</td><td>An integer</td></tr>
 * <tr><td>APPLICATION/APPADDR/SERVICE</td><td>0 or 1</td><td>Indicates
 * whether traffic between the client and the server should be encrypted
 * <td>993 indicates that secure IMAP4 over SSL should be used as described
 * in RFC2595.  143 indicates that an unsecured connection should be made
 * to the port number specified in the PORTNBR parameter.  If this parameter
 * is omitted, SSL is not used</td></tr>
 * <tr><td>APPLICATION/APPAUTH</td><td>0 or 1</td><td>Characteristic for
 * specifying the server's login credentials</td><td></td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHTYPE</td><td>0 or 1</td><td>
 * Indicates the type of authentication to be used</td><td>PLAIN, NTLM,
 * GSSAPI, CRAM-MD5, DIGEST-MD5, POPB4SMTP, APOP</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHNAME</td><td>0 or 1</td><td>
 * The user name</td><td> A string</td></tr>
 * <tr><td>APPLICATION/APPAUTH/AAUTHSECRET</td><td>0 or 1</td><td>
 * The password</td><td>A string</td></tr>
 * </table>
 * Note that all email APPLICATION characteristics must specify a server
 * address.  Those that do not will be discarded.  In addition SMTP accounts
 * must specify a FROM address.

 * An example of an OMA CP document that provisions an email account is
 * shown below.
\code
<wap-provisioningdoc version="1.0">
  <characteristic type="APPLICATION">
    <parm name="APPID" value="110"/>
    <parm name="PROVIDER-ID" value="localmail"/>
    <parm name="NAME" value="Local Email"/>
    <characteristic type="APPADDR">
      <parm name="ADDR" value="pop.localhost"/>
      <characteristic type="PORT">
	<parm name="PORTNBR" value="995"/>
	<parm name="SERVICE" value="995"/>
      </characteristic>
    </characteristic>
    <characteristic type="APPAUTH">
      <parm name="AAUTHNAME" value="user"/>
      <parm name="AAUTHSECRET" value="password"/>
    </characteristic>
  </characteristic>
  <characteristic type="APPLICATION">
    <parm name="APPID" value="25"/>
    <parm name="PROVIDER-ID" value="localmail"/>
    <parm name="NAME" value="Local Email"/>
    <parm name="FROM" value="user@localhost"/>
    <characteristic type="APPADDR">
      <parm name="ADDR" value="smtp.localhost"/>
      <characteristic type="PORT">
	<parm name="PORTNBR" value="465"/>
	<parm name="SERVICE" value="465"/>
      </characteristic>
    </characteristic>
    <characteristic type="APPAUTH">
      <parm name="AAUTHNAME" value="username"/>
      <parm name="AAUTHSECRET" value="password"/>
    </characteristic>
  </characteristic>
</wap-provisioningdoc>
\endcode
 *
 * It is converted into the following Provman keys by the CPClient.
 *
\code
/applications/email/localmail/name = Local Email
/applications/email/localmail/address = user@localhost
/applications/email/localmail/incoming/type = pop
/applications/email/localmail/incoming/host = pop.localhost
/applications/email/localmail/incoming/port = 995
/applications/email/localmail/incoming/usessl = always
/applications/email/localmail/incoming/password = password
/applications/email/localmail/incoming/username = user
/applications/email/localmail/outgoing/host = smtp.localhost
/applications/email/localmail/outgoing/port = 465
/applications/email/localmail/outgoing/username = username
/applications/email/localmail/outgoing/usessl = always
/applications/email/localmail/outgoing/type = smtp
/applications/email/localmail/outgoing/password = password
\endcode
 *

 */
