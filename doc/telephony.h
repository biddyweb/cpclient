/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file telephony.h
 * @brief Description of OMA CP telephony settings supported by the CPClient.
 *
 * @section telephony Telephony Settings
 *
 * OMA CP describes a very flexible and
 * powerful mechanism of provisioning connection settings.  Settings for
 * proxies and access points are configured separately.  Multiple
 * connectoids (access points or proxies) can be be associated with each
 * provisioned application and multiple access points can be associated with
 * each proxy.  Connection fallback is also supported.  Connectoids,
 * referenced by applications and proxies, are listed in order of priority. If
 * a connection cannot be made via the highest priority connectoid, the
 * subsequent is tried.
 *
 * This flexible connection model is not supported in today's smart phones.
 * Typically, such phones group proxy and access point settings together and
 * usually require only two sets of 3G connection settings (per SIM), one for
 * general Internet access and one for MMS.
 * For these reasons, the CPClient only implements a small subset
 * of OMA CP's connection rules and parameters.  A list of the supported
 * parameters is presented in the table below:
 *
 * <table>
 * <tr><th>OMA CP Parameter</th><th>Occurrence</th><th>Description</th>
 * <th>Permissible Values</th></tr>
 * <tr><td colspan="4" align="center"><i>Supported NAPDEF Parameters</i></td>
 * </tr>
 * <tr><td>NAPDEF/NAPID</td><td>1</td><td>Unique identifier for the access
 * point</td><td>Any string</td></tr>
 * <tr><td>NAPDEF/NAME</td><td>0 or 1</td><td>Friendly name for the access
 * point</td><td>Any string</td></tr>
 * <tr><td>NAPDEF/BEARER</td><td>1</td><td>Indicates the bearer.  Must be
 * specified as this parameter is mandatory for OMA CP but the CPClient does not
 * currently use it</td>
 * <td>GSM-CSD, GSM-GPRS</td></tr>
 * <tr><td>NAPDEF/INTERNET</td><td>0 or 1</td><td>Indicates the access point is
 * generic.  See below.</td><td>N/A</td></tr>
 * <tr><td>NAPDEF/NAPADDRESS</td><td>1</td><td>The address of the access point.
 * <td>Any string</td></tr>
 * <tr><td>NAPDEF/NAPAUTHINFO</td><td>0 or 1</td><td>Characteristic used to
 * specify the security credentials of a NAPDEF</td><td></td></tr>
 * <tr><td>NAPDEF/NAPAUTHINFO/AUTHTYPE</td><td>1</td><td>Type of authentication
 * used.  Must be specified as this parameter is mandatory for OMA CP but the
 * CPClient does not currently use it</td>
 * <td>PAP or CHAP</td></tr>
 * <tr><td>NAPDEF/NAPAUTHINFO/AUTHNAME</td><td>0 or 1</td><td>User name</td>
 * <td>Any string</td></tr>
 * <tr><td>NAPDEF/NAPAUTHINFO/AUTHSECRET</td><td>0 or 1</td><td>Password</td>
 * <td>Any string</td></tr>
 * <tr><td colspan="4" align="center"><i>Supported PXLOGICAL/PXPHYSICAL
 * Parameters</i></td> </tr>
 * <tr><td>PXLOGICAL/PXPHYSICAL</td><td>1</td>
 * <td>Characteristic that contains information about the physical proxy</td>
 * <td></td></tr>
 * <tr><td>PXLOGICAL/PXPHYSICAL/PXADDR</td><td>1</td><td>Proxy address</td>
 * <td>Any string (typically and IP address)</td></tr>
 * <tr><td>PXLOGICAL/PXPHYSICAL/PORT</td><td>0 or 1</td>
 * <td>Characteristic containing information about the proxy's type and port
 * </td><td></td></tr>
 * <tr><td>PXLOGICAL/PXPHYSICAL/PORT/PORTNBR</td><td>0 or 1</td><td>Proxy’s
 * port number</td> <td>An integer</td></tr>
 * <tr><td>PXLOGICAL/PXPHYSICAL/PORT/SERVICE</td><td>0 or 1</td>
 * <td>Proxy's type</td> <td>http, https or ftp</td></tr>
 * <tr><td colspan="4" align="center"><i>Supported MMS Parameters</i></td>
 * </tr>
 * <tr><td>APPLICATION/APPID</td><td>1</td><td>Identifies the application
 * characteristic as a group of MMS settings.</td><td>w4</td></tr>
 * <tr><td>APPLICATION/ADDR</td><td>1</td><td>Address of the MMS service
 * centre</td><td>A URL</td></tr>
 * <tr><td>APPLICATION/TO-PROXY</td><td>0 or 1</td><td>Reference to a logical
 * proxy defined elsewhere in the OMA CP document.</td>
 * <td>The PROXY-ID of a PXLOGICAL</td></tr>
 * <tr><td>APPLICATION/TO-NAPID</td><td>0 or 1</td><td>Reference to a
 * NAPDEF defined elsewhere in the OMA CP document.</td>
 * <td>The NAPID of a NAPDEF
 *  </td></tr>
 * </table>
 *
 * If you are familiar with the OMA CP specifications you will notice that
 * many of the OMA CP connection parameters are missing.  This reflects the
 * fact that only a subset of the OMA CP connection rules are implemented
 * by the CPClient.  It should be noted however, that the CPClient's OMA CP
 * document parser implements almost all the rules, characteristics and
 * parameters described in the OMA CP specifications.  It accepts an OMA CP
 * document and generates a complete OMA CP model in memory.  However, the
 * code inside the CPClient that maps this internal model to Provman keys
 * only uses the parameters listed in the table above.  It ignores all the
 * other parameters even though they are present in the in-memory model.  This
 * is mentioned here to publicise the fact that it would be possible to use the
 * code in lib directory of the CPClient to create an OMA CP implementation more
 * faithful to the standards, if anyone ever felt the need to do so.

 * The specific restrictions that exist are discussed in the paragraphs below.
 *
 * The CPClient groups settings for proxies and access points together.  The
 * identifiers used to represent a group of connection settings are derived
 * from the NAPID of the NAPDEF that specifies the access point.  For example,
 * the CPClient generates the following Provman keys
\code
/telephony/contexts/nd1/apn = nap.address
/telephony/contexts/nd1/password = password
/telephony/contexts/nd1/name = test
/telephony/contexts/nd1/username = username
/telephony/contexts/nd1/http_proxy = 127.0.0.1:8080
\endcode
 from the OMA CP document below
\code
<wap-provisioningdoc version="1.0">
	<characteristic type="NAPDEF">
		<parm name="NAME" value="test" />
		<parm name="NAPID" value="nd1" />
		<parm name="NAP-ADDRESS" value="nap.address" />
		<parm name="BEARER" value="GSM-GPRS" />
		<parm name="NAP-ADDRTYPE" value="APN" />
		<characteristic type="NAPAUTHINFO">
			<parm name="AUTHTYPE" value="PAP" />
			<parm name="AUTHNAME" value="username" />
			<parm name="AUTHSECRET" value="password" />
		</characteristic>
	</characteristic>

	<characteristic type="PXLOGICAL">
		<parm name="NAME" value="pxtest" />
		<parm name="PROXY-ID" value="logical" />
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical" />
			<parm name="PXADDR" value="127.0.0.1" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
			</characteristic>
		</characteristic>
	</characteristic>
</wap-provisioningdoc>
\endcode

 * Binding application settings to connection settings is not supported.
 * Neither is connection fallback.  TO-PROXY and TO-NAPID
 * references are ignored in all application characteristics apart from MMS
 * (see below).
 *
 * Two different types of connection settings are defined.  Settings for
 * MMS and generic settings for browsing the Internet.  The first set
 * of connection settings referenced directly or indirectly by the MMS first
 * application characteristic are deemed to be MMS connection settings.  The
 * CPClient generates a different set of Provman keys for such settings.  For
 * example, the CPClient generates the following keys
\code
/telephony/mms/proxy = 127.0.0.1:8080
/telephony/mms/apn = nap.address
/telephony/mms/mmsc = http://mms.myoperator
/telephony/mms/name = test
\endcode

for the OMA CP document below.
\code
<wap-provisioningdoc version="1.0">
	<characteristic type="NAPDEF">
		<parm name="NAME" value="test" />
		<parm name="NAPID" value="nd1" />
		<parm name="NAP-ADDRESS" value="nap.address" />
		<parm name="BEARER" value="GSM-GPRS" />
		<parm name="NAP-ADDRTYPE" value="APN" />
	</characteristic>

	<characteristic type="PXLOGICAL">
		<parm name="NAME" value="proxy" />
		<parm name="PROXY-ID" value="logical" />
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical" />
			<parm name="PXADDR" value="127.0.0.1" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
			</characteristic>
		</characteristic>
	</characteristic>

	<characteristic type="APPLICATION">
		<parm name="APPID" value="w4" />
		<parm name="TO-PROXY" value="logical" />
		<parm name="ADDR" value="http://mms.myoperator" />
	</characteristic>

</wap-provisioningdoc>
\endcode
 * Note that the settings are stored under /telephony/mms and not
 * /telephony/contexts.
 *
 * Connection settings associated with the MMS application cannot normally
 * be used by other applications to browse the Internet.  If, for some reason,
 * you wish to use the same connection settings for MMS and Internet browsing
 * you need to set the INTERNET parameter in the NAPDEF characteristic.  This
 * will cause two groups of settings to be created, one for MMS and one for
 * Internet, e.g., adding &lt;param name="INTERNET /&gt; to the NAPDEF above
 * would cause the following keys to be generated.

\code
/telephony/contexts/nd1/apn = nap.address
/telephony/contexts/nd1/name = test
/telephony/contexts/nd1/http_proxy = 127.0.0.1:8080
/telephony/mms/proxy = 127.0.0.1:8080
/telephony/mms/apn = nap.address
/telephony/mms/mmsc = http://mms.myoperator
/telephony/mms/name = test
\endcode
 *
 * Note the proxy is used for both set of connection settings.  If this is not
 * what you want you will need to maintain separate NAPDEFS for MMS and Internet
 * settings.
 *
 * Only the first valid physical proxy inside a logical proxy is used.  All
 * others are ignored.
 *
 * Only three types of proxies are supported, HTTP, HTTPS and FTP.  The proxy
 * type is deduced from the service parameter if specified or in the cases
 * where it is not from the port number.  Unknown port numbers maps to HTTP.
 *
 * To specify multiple proxy types for a 3G connection multiple PXLOGICALS
 * must be used.  Only a single instance of each type of proxy can be
 * associated with an access point (NAPDEF).  For example, if the document
 * contains multiple PXPHYSICAL characteristics that associate a HTTP proxy
 * with a single NAPDEF,  the first proxy will be used and all subsequent
 * proxies will be ignored.  To clarify, the OMA CP document below,
\code
<wap-provisioningdoc version="1.0">
	<characteristic type="NAPDEF">
		<parm name="NAME" value="test" />
		<parm name="NAPID" value="nd1" />
		<parm name="NAP-ADDRESS" value="nap.address" />
		<parm name="BEARER" value="GSM-GPRS" />
		<parm name="INTERNET"/>
		<parm name="NAP-ADDRTYPE" value="APN" />
	</characteristic>

	<characteristic type="PXLOGICAL">
		<parm name="NAME" value="proxy" />
		<parm name="PROXY-ID" value="logical" />
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical" />
			<parm name="PXADDR" value="127.0.0.1" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
			</characteristic>
		</characteristic>
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical2" />
			<parm name="PXADDR" value="127.1.1.1" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
				<parm name="SERVICE" value="https" />
			</characteristic>
		</characteristic>
	</characteristic>

	<characteristic type="PXLOGICAL">
		<parm name="NAME" value="proxy2" />
		<parm name="PROXY-ID" value="logical2" />
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical" />
			<parm name="PXADDR" value="127.2.2.2" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
				<parm name="SERVICE" value="https" />
			</characteristic>
		</characteristic>
	</characteristic>

	<characteristic type="PXLOGICAL">
		<parm name="NAME" value="proxy3" />
		<parm name="PROXY-ID" value="logical3" />
		<characteristic type="PXPHYSICAL">
			<parm name="PHYSICAL-PROXY-ID" value="physical" />
			<parm name="PXADDR" value="127.3.3.3" />
			<parm name="TO-NAPID" value="nd1" />
			<characteristic type="PORT">
				<parm name="PORTNBR" value="8080" />
				<parm name="SERVICE" value="https" />
			</characteristic>
		</characteristic>
	</characteristic>

</wap-provisioningdoc>
\endcode
 * generates the following Provman keys:
\code
/telephony/contexts/nd1/apn = nap.address
/telephony/contexts/nd1/name = test
/telephony/contexts/nd1/http_proxy = 127.0.0.1:8080
/telephony/contexts/nd1/https_proxy = 127.2.2.2:8080
\endcode
 *
 * The important thing to note here is that the HTTPS proxy used is the one
 * specified in the second PXLOGICAL.  This is because we only support one
 * PXPHYSICAL characteristic per PXLOGICAL.  So even though the first PXLOGICAL
 * contains a PXPHYSICAL that specifies a HTTPS proxy, this proxy is ignored,
 * as it is specified in the second PXPHYSICAL.  The third proxy is ignored
 * as it occurs further down in the document and hence has a lower priority
 * than the second.
 *
 * ACCESS rules are not supported.
 *
 * There are a number of restrictions placed on the provisioning of MMS
 * settings.
 *
 * Multiple sets of Internet access point/proxy combinations can be
 * provisioned.  However, only a single set of MMS connection settings are
 * supported.  This is evident from looking at the Provman keys above.
 * So even though OMA CP documents can contain multiple MMS applications
 * characteristics, the CPClient will only use the first of such characteristics
 * that appears in the document.
 *
 * In OMA CP, application characteristics can have multiple connectoids.
 * However, the CPClient will only use the first connectoid it encounters
 * in an MMS application characteristic, be it a NAPDEF or a PXLOGICAL.  If
 * it is a NAPDEF no proxy information will be used to access the MMSC.
 * If the connectoid is a PXLOGICAL characteristic, the access point settings
 * used to connect to the MMS server will be the those of the first NAPDEF
 * associated with the first physical proxy of the PXLOGICAL.  As only a single
 * connectoid is used with an MMS characteristic, it follows that only a single
 * proxy can be specified for use with MMS, this being
 * the proxy specified in the first physical proxy of the logical proxy
 * referenced by the MMS characteristic.
 */
