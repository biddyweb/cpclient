/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file push-message.h
 * \brief Documentation for the com.intel.cpclient.PushMessage interface
 *
 * All the methods documented below are part of the
 * \a com.intel.cpclient.PushManager interface.
 */

/*!
 * \brief Applies the settings contained within the Push Message object
 *
 * This method applies all the settings contained inside the Push Message.
 * It does this by generating a set of Provman settings and meta data
 * values, initiating a Provman session and passing all the generated
 * settings and values to Provman to be applied.
 *
 * The Push Message object is authenticated before the settings are
 * applied.  Most authentication methods require a PIN code, which needs
 * to be passed by the CP UI application as an argument to this method.
 * If the security method used by the message does not require
 * a pin code to authenticate, i.e., it is using NETWPIN authentication,
 * an empty string should be passed.  The UI can determine whether or
 * not a PIN code is required by calling the Push Message's GetProps
 * method.  If authentication fails, the UI can call the #Apply method
 * again with a different PIN code.  No limits are placed on the number
 * of attempts that can be made.  If such a limit is required it needs to
 * be implemented in the UI.
 *
 * All OMA CP security mechanisms apart from no security are supported.
 * Messages that contain no security information cannot be applied.
 * Requirements to not support certain security mechanisms, such as NETWPIN,
 * need to be implemented in the OMA CP UI application.  The UI application
 * can invoke the #GetProps method of the Push Message object to determine the
 * security mechanism used by that object.
 *
 * Once a message has been successfully applied, the PushMessage object stays in
 * existence, until its #Close Method is called.  However, any further calls to
 * #Apply will result in an error.
 *
 * @param pin_code a string containing the pin_code to authenticate the
 * message.
 * \exception com.intel.cpclient.Error.Died The CPClient was killed before
 *   the #Apply command could be executed.
 * \exception com.intel.cpclient.Error.Cancelled The CPClient was killed before
 *   the #Apply command could be completed.
 * \exception com.intel.cpclient.Error.OOM Not enough memory was available
 * to process the message.
 * \exception com.intel.cpclient.Error.IO Unable to communicate with Provman.
 * \exception com.intel.cpclient.Error.Denied The message could not be
 * authenticated or the caller did not create the message.
 * \exception com.intel.cpclient.Error.AlreadyApplied The message has already
 * been applied.
 * \exception com.intel.cpclient.Error.NotFound The specified Push Message does
 * not exist
*/

void Apply(string pin_code);

/*!
 * \brief Discards the Push Message Object.
 *
 * The message is discarded and any settings it contains will be lost if the
 * message has not already been applied.
 *
 * \exception com.intel.cpclient.Error.Denied The caller is not the creator of
 * the Push Message object.
 * \exception com.intel.cpclient.Error.NotFound The specified Push Message does
 * not exist
 *
*/

void Close();

/*!
 * \brief Retrieves the properties of the Push Message.
 *
 * This method returns a dictionary containing one instance of each
 * of the key/value pairs described in the table below.
 * <table>
 * <tr><Th>Key</th><th>Description</th><th>Possible Values</th></tr>
 * <tr><td>SecType</td><td>The type of security used by the
 * Push Message object</td><td>NONE, USERPIN, NETWPIN, USERNETWPIN, USERPINMAC
 * </td></tr>
 * <tr><td>PinRequired</td><td>Indicates whether or not a PIN code is required
 * to authenticate this PushMessage</td><td>Yes, No</td></tr>
 * <tr><td>StartSessionsWith</td><td>A list of DM servers with which the device
 * should initiate DM sessions after it has applied the Push Message</td>
 * <td>A comma separated list of DM server IDs.</td></tr>
 * <tr><td>Settings</td><td>Provides an overview of the settings contained
 * within the Push Message</td>
 * <td>A '/' separated list of setting types.  The following types are
 * defined: proxy, apn, bookmarks, email, mms, omadm, omads</td></tr>
 * </table>
 *
 * \exception com.intel.cpclient.Error.Died The CPClient was killed before
 *   the #GetProps command could be executed.
 * \exception com.intel.cpclient.Error.Denied The caller is not the creator of
 * the Push Message object.
 * \exception com.intel.cpclient.Error.NotFound The specified Push Message does
 * not exist
*/

dictionary GetProps(string pin_code);

