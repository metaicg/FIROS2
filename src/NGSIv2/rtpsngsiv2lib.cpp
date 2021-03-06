#include <iostream>
#include <tinyxml2.h>
#include "ISBridgeFastRTPSToNGSIv2.h"

#if defined(_WIN32) && defined (BUILD_SHARED_LIBS)
	#if defined (_MSC_VER)
		#pragma warning(disable: 4251)
	#endif
  #if defined(rtpsngsiv2lib_EXPORTS)
  	#define  USER_LIB_EXPORT __declspec(dllexport)
  #else
    #define  USER_LIB_EXPORT __declspec(dllimport)
  #endif
#else
  #define USER_LIB_EXPORT
#endif

ISBridgeFastRTPSToNGSIv2* loadFastRTPSToNGSIv2Bridge(tinyxml2::XMLElement *bridge_element);

extern "C" ISBridge* USER_LIB_EXPORT createBridge(tinyxml2::XMLElement *bridge_element)
{
    return loadFastRTPSToNGSIv2Bridge(bridge_element);
}

tinyxml2::XMLElement* _assignNextElement(tinyxml2::XMLElement *element, std::string name){
    if (!element->FirstChildElement(name.c_str())){
        throw 0;
    }
    return element->FirstChildElement(name.c_str());
}

tinyxml2::XMLElement* _assignOptionalElement(tinyxml2::XMLElement *element, std::string name){
    return element->FirstChildElement(name.c_str());
}

ISBridgeFastRTPSToNGSIv2* loadFastRTPSToNGSIv2Bridge(tinyxml2::XMLElement *bridge_element)
{
    try
    {
        tinyxml2::XMLElement *fastrtps_element = bridge_element->FirstChildElement("fastrtps");
        if (!fastrtps_element)
        {
            fastrtps_element = bridge_element->FirstChildElement("ros2");
            if (!fastrtps_element)
            {
                fastrtps_element = bridge_element->FirstChildElement("subscriber"); // Case unidirectional configuration
            }
        }
        tinyxml2::XMLElement *ngsiv2_element = bridge_element->FirstChildElement("ngsiv2");
        if (!ngsiv2_element)
        {
            ngsiv2_element = bridge_element->FirstChildElement("publisher"); // Case unidirectional configuration
        }
        if(!fastrtps_element || !ngsiv2_element)
        {
            throw 0;
        }

        tinyxml2::XMLElement *current_element;
        current_element = _assignNextElement(fastrtps_element, "participant");
        const char* fastrtps_participant_name = current_element->GetText();
        current_element = _assignNextElement(fastrtps_element, "topic");
        const char* fastrtps_topic_name = current_element->GetText();
        current_element = _assignNextElement(fastrtps_element, "type");
        const char* fastrtps_type_name = current_element->GetText();

        current_element = _assignOptionalElement(fastrtps_element, "partition");
        const char* fastrtps_partition = (current_element == nullptr) ? nullptr : current_element->GetText();

        current_element = _assignNextElement(ngsiv2_element, "participant");
        const char* ngsiv2_participant_name = current_element->GetText();
        current_element = _assignNextElement(ngsiv2_element, "id");
        const char* ngsiv2_id = current_element->GetText();

        current_element = _assignOptionalElement(ngsiv2_element, "host");
        const char* ngsiv2_host = (current_element == nullptr) ? "localhost" : current_element->GetText();

        int fastrtps_domain = 0;
        current_element = _assignNextElement(fastrtps_element, "domain");
        if(current_element->QueryIntText(&fastrtps_domain))
        {
            throw 0;
        }

        int ngsiv2_port;
        current_element = _assignNextElement(ngsiv2_element, "port");
        if(current_element->QueryIntText(&ngsiv2_port))
        {
            ngsiv2_port = 1026;
        }

        const char* function_path;
        if (bridge_element->FirstChildElement("transformation"))
        {
            function_path = bridge_element->FirstChildElement("transformation")->GetText();
        }
        else
        {
            if (bridge_element->FirstChildElement("transformToNGSIv2"))
            {
                function_path = bridge_element->FirstChildElement("transformToNGSIv2")->GetText();
            }
            else
            {
                function_path = nullptr;
                std::cout << "ERROR: No transformation function defined." << std::endl;
                throw 0;
            }
        }

        // Participant (publisher) configuration
        ParticipantAttributes participant_fastrtps_params;
        participant_fastrtps_params.rtps.builtin.domainId = fastrtps_domain;
        participant_fastrtps_params.rtps.builtin.leaseDuration = c_TimeInfinite;
        participant_fastrtps_params.rtps.setName(fastrtps_participant_name);

        // Publisher configuration
        PublisherAttributes publisher_params;
        publisher_params.historyMemoryPolicy = DYNAMIC_RESERVE_MEMORY_MODE;
        publisher_params.topic.topicDataType = fastrtps_type_name;
        publisher_params.topic.topicName = fastrtps_topic_name;
        if (fastrtps_partition != nullptr)
        {
            publisher_params.qos.m_partition.push_back(fastrtps_partition);
        }

        // Subscriber configuration
        SubscriberAttributes subscriber_params;
        subscriber_params.historyMemoryPolicy = DYNAMIC_RESERVE_MEMORY_MODE;
        subscriber_params.topic.topicKind = NO_KEY;
        subscriber_params.topic.topicDataType = fastrtps_type_name;
        subscriber_params.topic.topicName = fastrtps_topic_name;
        if (fastrtps_partition != nullptr)
        {
            subscriber_params.qos.m_partition.push_back(fastrtps_partition);
        }

        // NGSIv2 configuration
        NGSIv2Params participant_ngsiv2_params;
        participant_ngsiv2_params.name = ngsiv2_participant_name;
        participant_ngsiv2_params.host = ngsiv2_host;
        participant_ngsiv2_params.idPattern = ngsiv2_id;
        participant_ngsiv2_params.port = ngsiv2_port;

        ISBridgeFastRTPSToNGSIv2 *rtps_ngsiv2 = new ISBridgeFastRTPSToNGSIv2(participant_fastrtps_params,
                            participant_ngsiv2_params,
                            subscriber_params,
                            function_path);

        return rtps_ngsiv2;
    }
    catch (int e_code){
        std::cout << "Invalid configuration, skipping bridge " << e_code << std::endl;
        return nullptr;
    }
}
