/******************************************************************************
 * Copyright (C) 2011  Intel Corporation. All rights reserved.
 *****************************************************************************/
/*!
 * @file manager.h
 * \brief Documentation for the com.intel.cpclient.Manager interface
 *
 * All the methods documented below are part of the
 * \a com.intel.cpclient.Manager interface implemented by the
 * \a /com/intel/cpclient object.
 */

/*!
 * \brief Returns the version number of the CPClient
 *
 * @return Returns the current version number as a string.
 *
 * \exception com.intel.cpclient.Error.Died The CPClient was killed before
 *   the #GetVersion command could be executed.
*/

void GetVersion();

/*!
 * \brief Creates a com.intel.cpclient.PushMessage object from
 * an OMA CP WAP Push message.
 *
 * This method extracts and parses the WAP Push headers and the WBXML
 * encoded OMA CP document contained within the array of bytes passed to it by
 * the caller. A new d-Bus
 * object is created to represent this decoded message, the path of
 * which is returned to the caller.  If this message is corrupt
 * or a problem is encountered while parsing it, an error is returned
 * and the d-Bus object is not created.  The new d-Bus object implements
 * the com.intel.cpclient.PushMessage message and can be used to
 * inspect and apply the contents of the WAP Push message.
 *
 * Please note that this function does not cause any of the settings contained
 * in the WAP Push message to be applied, i.e., passed to Provman.  It
 * merely decodes the message and creates a new object that allows the caller
 * to inspect the message and apply it if he sees fit.
 *
 * @param message the binary contents of an OMA CP WAP Push message
 *  including the WSP headers.
 * @return the path of the newly created d-Bus object.
 *
 * \exception com.intel.cpclient.Error.Died The CPClient was killed before
 *   the #GetVersion command could be executed.
 * \exception com.intel.cpclient.Error.OOM Not enough memory was available
 * to process the message.
 * \exception com.intel.cpclient.Error.ParseError The contents of the message
 * are not well formed.
 * \exception com.intel.cpclient.Error.IO Unable to register the new object
 * with the session d-Bus.
*/

path CreatePushMessage(array message);

/*!
 * \brief Test function to Parse and apply the contents of an OMA CP XML file.
 *
 * This method is purely intended for testing purposes.  It loads and
 * parses the XML (not WBXML) and applies all the settings contained
 * within that file.  The message is applied directly without any
 * authentication.  As the IMSI card is not used during authentication,
 * the Provman session used to provision the document's settings is
 * initiated with the empty string, informing Provman that it should associate
 * any SIM specific settings with the SIM card present in the first modem
 * reported by the device.
 *
 * @param filename The full path to the OMA CP XML file to be processed.
 *
 * \exception com.intel.cpclient.Error.Died The CPClient was killed before
 *   the #GetVersion command could be executed.
 * \exception com.intel.cpclient.Error.OOM Not enough memory was available
 * to process the message.
 * \exception com.intel.cpclient.Error.ParseError The contents of the message
 * are not well formed.
 * \exception com.intel.cpclient.Error.LoadFailed Unable to load filename.
*/

void ParseCP(string filename);
