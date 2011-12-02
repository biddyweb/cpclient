/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file mainpage.h
 * @mainpage CPClient
 * @section devman Device Management and Provisioning
 *
 * Device management refers to technologies that allow authorised
 * third parties, typically an enterprise or a network operator, to
 * remotely configure and manage an end user's device. There are a number
 * of well established use cases, including provisioning of application
 * and connection settings, remotely locking and wiping the device, and
 * updating the device's firmware. One of the
 * original use cases for device management on mobile phones was over the
 * air provisioning, or OTA provisioning for short. Devices ship with
 * pre-installed applications that provide users with access to various
 * services, such as the Internet, the ability to send MMS messages, the
 * ability to backup and restore their devices, etc. These applications
 * require operator specific settings to work, e.g., the address of the
 * operator’s MMS service centre. Provisioning is the name given to the
 * process by which devices are provided with the settings they need to
 * function in their host network.

 * Device settings could of course be burnt onto the device at factory
 * time, but this only works if the device is locked to a specific
 * operator in a specific geography, and the operator settings do not
 * change over time. In addition, factory provisioning cannot be used for
 * account based settings, such as email accounts, that differ from one
 * user to another. For these reasons, the mobile phone industry has
 * developed some standards that allow settings to be provisioned Over
 * The Air (OTA) after the device has left the factory. The Open Mobile
 * Alliance (OMA) maintains two standards that can be used to provision
 * settings over the air. These are OMA Client Provisioning (OMA CP) and
 * OMA Device Management (OMA DM). The CPClient, implements the first
 * of these standards, OMA CP.
 *
 * @section omacp OMA Client Provisioning
 * OMA Client Provisioning can
 * be used to provision a range of application types, such as browser,
 * SyncML, IMPS, MMS, AGPS, and email.  OMA CP settings are placed in
 * an XML document, WBXML encoded and sent
 * to the device via a WAP Push message. OMA CP messages need to be
 * authenticated on the client before the settings they contain can be
 * applied. Authentication is based on the existence of a shared
 * secret. This can be a PIN code supplied to the user by the operator
 * before the message is sent, or it can be based on some piece of SIM
 * specific data known to both the operator and the device, e.g., the
 * IMSI.
 *
 * A typical OMA CP use case is as follows. A user gets a new phone and
 * SIM card. He discovers that he cannot connect to the Internet or send
 * MMS messages with his new SIM card. He goes to the operator's web
 * site, navigates to the configuration page, selects his phone model,
 * enters his phone number and presses a button. The web page displays a
 * PIN code which the user notes down.
 * A few seconds later the user should receive a
 * notification on his device.  When he clicks on the notification he will be
 * informed that new settings have been received and he will be asked for the
 * pin code.  Once he enters the pin code and clicks ok, the settings will be
 * applied.  Having accepted the settings the user should discover that he can
 * now browse the Internet and send MMS messages.
 *
 * For more information about OMA CP the reader is referred to the
 * <a href=
"http://www.openmobilealliance.org/Technical/release_program/cp_v1_1.aspx">
 * OMA CP Standards </a>
 *
 * @section cpclient The CPClient
 * The CPClient is a middleware component that provides support for over the air
 * provisioning of application and connection settings.
 * It does not provide a complete OMA CP provisioning solution.  For
 * example, it does not include a UI to display the details of a newly received
 * provisioning message.  Nor does it
 * include code to listen for the arrival of such messages. In
 * addition, the CPClient does not directly modify application and connection
 * settings.  Rather, it delegates these tasks to another component called
 * <a href="https://github.com/otcshare/provman">Provman</a>.
 * The reason
 * that this functionality is not included in the CPClient is that the CPClient
 * is designed to be portable and unfortunately, user interfaces and the APIs
 * for receiving WAP Push messages and configuring device settings, tend to be
 * very platform specific.
 *
 * So, having said all this, the reader may forgiven
 * for wondering what exactly it is that the CPClient does. Well in short, it
 * exposes d-Bus APIs that offer the following functionality:
 * <ul>
 * <li> Authenticate OMA CP WAP Push messages</li>
 * <li> Provide clients with a summary of OMA CP message contents</li>
 * <li> Apply the settings contained in OMA CP messages</li>
 * </ul>
 *
 * To integrate the CPClient into your platform you need to perform two steps:
 * <ol>
 * <li> Create an application that listens for OMA CP WAP Push messages and that
 * implements the OMA CP UI.</li>
 * <li> Create Provman plugins for your platform if they do not already exist.
 * </li>
 * </ol>
 *
 * In step one the integrator will create an application that registers to
 * receive OMA CP WAP push messages using the available platform specific API.
 * When a message is received, the
 * application will pass it in its entirety to the CPClient.  It can then
 * use the CPClient's APIs to determine the contents of the message and whether
 * or not a PIN code is required to authenticate the message.  The application
 * can present this information to the user and ask him to enter a PIN code
 * if necessary.  If the user chooses to accept the message, all the
 * applications needs to do is to ask the CPClient to apply the message.  The
 * CPClient will do so by converting the contents of the message into a set of
 * key value pairs which it will pass to
 * <a href="https://github.com/otcshare/provman">Provman</a>.
 *  <a href="https://github.com/otcshare/provman">Provman</a>.
 * in turn will store the settings in the appropriate data stores for the
 * device.
 *
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * is a middleware component that facilitates the task of writing device
 * management clients.  It permits multiple device management clients running
 * on the same device to communicate and it also prevents them from
 * interfering with each other.
 * However, it's most important feature is that it isolates
 * device management clients from the underlying middleware.  Knowledge about
 * how to create an email account or a 3G connection setting is coded into
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * and not into the individual device management clients.  This reduces
 * code duplication between the various clients and it makes
 * them very portable.
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * does not itself actually know how
 * to provision an email account on a given device.
 * Instead this information is encoded into its plugins.
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * supports a number of different types of plugins.  There are telephony
 * plugins for provisioning telephony settings, email plugins for
 * email accounts and data synchronisation plugins for OMA DS accounts.
 * When porting
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * to your platform you simply need to ship it with the set of plugins
 * suitable for your platform.  If these plugins do not already exist
 * you will need to write them.  Creating new
 * <a href="https://github.com/otcshare/provman">Provman</a>  plugins is the
 * second step of porting the CPClient to your platform.
 *
 * @section settings Supported Settings
 * The CPClient currently supports the provisioning of the
 * following settings:
 * <ul>
 * <li> \ref telephony (MMS, APNs and Proxies) </li>
 * <li> \ref browser</li>
 * <li> \ref synchronisation </li>
 * <li> \ref omadm </li>
 * <li> \ref email (IMAP, POP3 and SMTP) </li>
 * </ul>
 *
 * @section sim-specific  SIM Specific Settings
 *
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * regards some settings as SIM specific.
 * This is necessary as certain middleware components,
 * particularly telephony components such as
 * <a href="http://ofono.org">oFono</a>, maintain separate groups
 * of settings for each SIM card that has been inserted into the device.
 * They do this because telephony  settings provisioned for one
 * SIM card may not work when another SIM card is in use,
 * particularly if the new SIM card is issued by a different network operator.
 * The CPClient and <a href="https://github.com/otcshare/provman">Provman</a>
 *  are aware of this peculiarity and cooperate with
 * the underlying middleware to ensure that
 * settings provisioned for operator A are not used when operator B's SIM card
 * is present in the device.
 *
 * When the CPClient processes an OMA CP document it initiates a device
 * management session with
 * <a href="https://github.com/otcshare/provman">Provman</a> to whom it
 * passes the set of key/value pairs that it has extracted from the document.
 * An example of this process can be seen in \ref browser.
 * When initiating a device management session, the CPClient also passes
 * <a href="https://github.com/otcshare/provman">Provman</a> the IMSI number
 * that was used to authenticate the WAP Push message. If no IMSI number was
 * required to authenticate the message, an empty string is passed to
 * <a href="https://github.com/otcshare/provman">Provman</a> in its place.
 * This indicates to Provman that any SIM specific settings
 * provisioned during the management session should be associated with the
 * IMSI number of the first modem discovered in the device.
 * <a href="https://github.com/otcshare/provman">Provman</a>
 * uses this IMSI number to bind any SIM specific settings to the
 * appropriate SIM card.
 * @section unsupported  Unsupported OMA CP Characteristics
 * The following OMA CP characteristics are not supported.
 * <ul>
 * <li>ACCESS</li>
 * <li>BOOTSTRAP</li>
 * <li>CLIENTIDENTITY</li>
 * <li>VALIDITY</li>
 * <li>VENDORCONFIG</li>
 * </ul>
 * @section dbus The d-Bus API
 * Each Provman instance registers the name
 * \a com.intel.cpclient.server on the session D-Bus bus.  On start up,
 * the server exposes a single D-Bus object, \a /com/intel/cpclient that
 * implements a single interface, \a com.intel.cpclient.Manager.
 * \a com.intel.cpclient.Manager exposes a method called #CreatePushMessage.
 * OMA CP UI applications should call this method when they receive a new
 * OMA CP WAP Push message, passing it the binary contents of the
 * message as an argument.  If the message is valid, the CPClient will create a
 * new object returning the path of this object to the calling UI as a return
 * value.  This new object implements a second interface called,
 * \a com.intel.cpclient.PushMessage that supports 3 methods,
 * #Apply, #GetProps and #Close.  These methods allow the UI to apply the OMA CP
 * message, to determine the properties of the message, i.e. the settings it
 * contains, the authentication method used, etc., and to delete the object,
 * respectively.  A simple example of using the d-Bus APIs in python is
 * given below.
\code
def parseWP(binaryMessage, pinCode):
    manager = dbus.Interface(bus.get_object('com.intel.cpclient.server',
                                            '/com/intel/cpclient'),
					    'com.intel.cpclient.Manager')
    path = manager.CreatePushMessage(binaryMessage)
    try:
	pm = dbus.Interface(bus.get_object('com.intel.cpclient.server', path),
					   'com.intel.cpclient.PushMessage')
	pm.Apply(pinCode)
	print "Message Applied"
    except dbus.exceptions.DBusException, err:
	print "Unable to process WAP Push message: ", err
    finally:
	pm.Close()
\endcode
* A complete list of all the d-Bus methods exposed by the CPClient are presented
* in the table below.  Click on the links for more detailed information.
 * \par
 * <table>
 * <tr><th>Method</th><th>Description</th></tr>
 * <tr><td colspan="2" align="center">
 * <i>Methods defined by com.intel.cpclient.Manager</i></td></tr>
 * <tr><td>#CreatePushMessage</td><td>\copybrief CreatePushMessage</td></tr>
 * <tr><td>#GetVersion</td><td>\copybrief GetVersion</td></tr>
 * <tr><td>#ParseCP</td><td>\copybrief ParseCP</td></tr>
 * <tr><td colspan="2" align="center">
 * <i>Methods defined by com.intel.cpclient.PushMessage</i></td>
 * </tr>
 * <tr><td>#Apply</td><td>\copybrief Apply</td></tr>
 * <tr><td>#GetProps</td><td>\copybrief GetProps</td></tr>
 * <tr><td>#Close</td><td>\copybrief Close</td></tr>
 * </table>
 *
 * @section overwrite Overwriting Settings
 * When a device management client is asked to provision an account or a group
 * of settings that already exist it is faced with a dilemma.  Should it
 * overwrite the existing settings with the new settings received over the air,
 * or should it simply discard the new settings and continue to use the old?
 * The OMA CP specifications recommend the latter.  However, this is often not
 * very useful as it prevents existing account settings from being updated over
 * the air via OMA CP.  To update an account you would need to provision an
 * entirely new account with a different account identifier and with the updated
 * settings.  The user will now have
 * two versions of the same account present on the device.  The updated
 * account which works and the old account which presumably does not.  This is
 * a little confusing.
 *
 * For this reason the CPClient supports an overwrite mode in addition to a
 * standard mode that implements the OMA CP recommended behaviour.  The
 * integrator must select the mode that he wants to use during the
 * compilation phase.  By default the CPClient overwrites existing accounts with
 * new accounts received over the air.  However, this behaviour can be disabled
 * by passing the --enable-overwrite="no" option to the configure script.
 *
 * When overwrite mode is used, all settings from an existing account
 * are deleted before the updated account is provisioned.
 * Settings from the old and new versions of the account are never mixed.
 * Overwrite mode also deletes existing versions of Internet access points
 * and bookmarks before provisioning the new versions, received over the air.
 *
 * The manner in which the CPClient identifies whether a particular group
 * of settings already exist or not depends on what is being provisioned.  For
 * OMADS, OMADM and Email accounts the PROVIDER-ID is used to match new settings
 * to old ones.  If no PROVIDER-ID is specified the NAME parameter is used.
 * The NAPID is used for matching access points and the NAME parameter is used
 * for browser bookmarks.
 *
 * For example, let us suppose that a device is provisioned with an Internet
 * access point whose NAPID is set to <i>cooloperator</i>, just after it leaves
 * the factory.  One year later the cool operator has changed the password
 * required to access its 3G network. It sends the device a new OMA CP message
 *  containing a NAPDEF definition for the updated access point.  The NAPID of
 * the new NAPDEF is the same as the old, i.e., <i>cool operator</i>.
 * If the CPClient is compiled to
 * user overwrite mode, the old access point will be completely deleted, before
 * the new access point is provisioned.  Once the process is finished the user
 * is left with a single access point with the updated settings, which is
 * probably want they want.  If overwrite mode is disabled, the contents of the
 * updated settings inside the new NAPDEF will be ignored and the old settings
 * will remain untouched.
 *
 * Overwrite mode is always used for MMS regardless of the
 * compile options passed to configure.  As only one instance of these settings
 * can exist, preventing overwrite of these settings would mean that they could
 * never be updated, which sort of defeats the purpose of over the air
 * provisioning.
 *
 * @section installing Compiling and Building
 * Please see the README file for details of how to build and install the
 * CPClient.
 * @section coding Coding Style
 * <a href="https://github.com/otcshare/provman">Provman's</a> coding style is
 * used in the CPClient with three exceptions/additions.
 * <ol>
 * <li>The prefixes cpc_ and CPC_ are used in place of provman_ and
 * PROVMAN.</li>
 * <li>The error macros defined in error-macros.h are used for error handling
 * instead of gotos.</li>
 * <li>An attempt has been made to keep the code in the lib directory as
 * portable as possible.  The only direct dependency that this code has is
 * on libxml2.  The code is isolated from all other dependencies by peer
 * layers.  Do not add any new dependencies to this code without defining
 * new peer layer functions.</li>
 * </ol>
 *
 * A typical error handling use case employing the error macros is shown below.
\code
int fn()
{
	DMC_ERR_MANAGE;

	DMC_FAIL(fn1());
	DMC_FAIL(fn1());

DMC_ON_FAIL:

	return DMC_ERR;
}
\endcode
 * DMC_ERR_MANAGE defines a local variable, DMC_ERR, and sets its value to 0,
 * i.e., no error. The DMC_FAIL macro executes a C expression and jumps to
 * DMC_ON_FAIL if the result of this macro is not 0. See  error-macros.h for
 * more details.

 ******************************************************************************/

