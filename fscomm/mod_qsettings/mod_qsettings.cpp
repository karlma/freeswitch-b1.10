/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2009, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Joao Mesquita <jmesquita@freeswitch.org>
 *
 *
 * Description:
 * Module to load configurations from Qt preference system QSettings
 *
 */
#include <QString>
#include <QtGui>
#include <QDir>
#include <QXmlStreamWriter>
#include "mod_qsettings/mod_qsettings.h"

switch_xml_t XMLBinding::getConfigXML(QString tmpl)
{
    _settings->beginGroup("FreeSWITCH/conf");
    if (!_settings->childGroups().contains(tmpl))
    {
        _settings->endGroup();
        return NULL;
    }
    _settings->beginGroup(tmpl);

    QByteArray *finalXML = new QByteArray();
    QXmlStreamWriter streamWriter(finalXML);

    streamWriter.setAutoFormatting(true);
    streamWriter.writeStartElement("document");
    streamWriter.writeAttribute("type", "freeswitch/xml");

    streamWriter.writeStartElement("section");
    streamWriter.writeAttribute("name", "configuration");

    streamWriter.writeStartElement("configuration");
    streamWriter.writeAttribute("name", tmpl);
    streamWriter.writeAttribute("description", "Configuration generated by QSettings");

    foreach (QString group, _settings->childGroups())
    {
        parseGroup(&streamWriter, group);
    }

    streamWriter.writeEndElement();
    streamWriter.writeEndElement();
    streamWriter.writeEndElement();

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Config for %s requested. Providing the following XML:\n%s\n",
                      tmpl.toAscii().constData(), finalXML->data());

    _settings->endGroup();
    _settings->endGroup();

    return switch_xml_parse_str(finalXML->data(), strlen(finalXML->data()));
}

void XMLBinding::parseGroup(QXmlStreamWriter *streamWriter, QString group)
{
    if (group == "attrs")
    {
        _settings->beginGroup(group);
        foreach (QString k, _settings->childKeys())
        {
            streamWriter->writeAttribute(k, _settings->value(k).toString());
        }
        _settings->endGroup();
        return;
    }

    if (group == "params" || group == "customParams")
    {
        _settings->beginGroup(group);
        foreach(QString param, _settings->childKeys())
        {
            streamWriter->writeStartElement("param");
            streamWriter->writeAttribute("name", param);
            streamWriter->writeAttribute("value", _settings->value(param).toString());
            streamWriter->writeEndElement();
        }
        _settings->endGroup();
        return;
    }

    if (group == "gateways")
    {
        streamWriter->writeStartElement(group);
        _settings->beginGroup(group);
        foreach (QString gw, _settings->childGroups())
        {
            _settings->beginGroup(gw);
            foreach(QString g, _settings->childGroups())
            {
                parseGroup(streamWriter, g);
            }
            _settings->endGroup();
        }
        _settings->endGroup();
        streamWriter->writeEndElement();
        return;
    }

    _settings->beginGroup(group);
    streamWriter->writeStartElement(group);

    foreach (QString group2, _settings->childGroups())
    {
        parseGroup(streamWriter, group2);
    }

    streamWriter->writeEndElement();
    _settings->endGroup();
}

static switch_xml_t xml_url_fetch(const char *section, const char *tag_name, const char *key_name, const char *key_value, switch_event_t *params,
                                  void *user_data)
{
    XMLBinding *binding = (XMLBinding *) user_data;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "We are being requested -> section: %s | tag_name: %s | key_name: %s | key_value: %s!\n",
                      section, tag_name, key_name, key_value);
    if (!binding) {
        return NULL;
    }

    return binding->getConfigXML(key_value);
}

static switch_status_t do_config(void)
{
    char *cf = "qsettings.conf";
    switch_xml_t cfg, xml, bindings_tag;
    XMLBinding *binding = NULL;

    if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
        return SWITCH_STATUS_TERM;
    }

    if (!(bindings_tag = switch_xml_child(cfg, "bindings"))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing <bindings> tag!\n");
        switch_xml_free(xml);
        return SWITCH_STATUS_FALSE;
    }

    QString bind_mask = switch_xml_attr_soft(bindings_tag, "value");
    if (!bind_mask.isEmpty())
    {
        binding = new XMLBinding(bind_mask);
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Binding XML Fetch Function [%s]\n",
                      binding->getBinding().isEmpty() ? "all" : binding->getBinding().toAscii().constData());
    switch_xml_bind_search_function(xml_url_fetch, switch_xml_parse_section_string(binding->getBinding().toAscii().constData()), binding);
    binding = NULL;

    switch_xml_free(xml);
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_qsettings_load(void)
{

    if (do_config() == SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Sucessfully configured.\n");
    } else {
        return SWITCH_STATUS_FALSE;
    }
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "We loaded mod_qsettings.\n");

    return SWITCH_STATUS_SUCCESS;
}
