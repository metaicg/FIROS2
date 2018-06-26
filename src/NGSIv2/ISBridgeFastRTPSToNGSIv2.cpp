// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/attributes/PublisherAttributes.h"

#include <fastrtps/Domain.h>

#include "ISBridgeFastRTPSToNGSIv2.h"
#include "idl/JsonNGSIv2PubSubTypes.h" // Type to send to NGSIv2

using asio::ip::tcp;

ISBridgeFastRTPSToNGSIv2::ISBridgeFastRTPSToNGSIv2(
                    ParticipantAttributes par_fastrtps_params,
                    NGSIv2Params par_ngsiv2_params,
                    SubscriberAttributes sub_params,
                    const char *file_path
                ) : mf_participant(nullptr),
                    mf_subscriber(nullptr),
                    data_type(nullptr),
                    ngsiv2_host(par_ngsiv2_params.host),
                    ngsiv2_port(par_ngsiv2_params.port),
                    ngsiv2_id(par_ngsiv2_params.idPattern),
                    ngsiv2_publisher(ngsiv2_host, ngsiv2_port),
                    m_listener(file_path)
{
    // Create RTPSParticipant
    mf_participant = Domain::createParticipant(par_fastrtps_params);
    if(mf_participant == nullptr) return;

    //Register types
    data_type = new GenericPubSubType();
    data_type->setName(sub_params.topic.topicDataType.c_str());
    Domain::registerType(mf_participant,(TopicDataType*) data_type);

    m_listener.setPublisher(&ngsiv2_publisher);

    // Create Subscriber
    mf_subscriber = Domain::createSubscriber(mf_participant,sub_params,(SubscriberListener*)&m_listener);
    if(mf_subscriber == nullptr) return;
}

ISBridgeFastRTPSToNGSIv2::~ISBridgeFastRTPSToNGSIv2(){
    if(mf_participant != nullptr) Domain::removeParticipant(mf_participant);
}

void ISBridgeFastRTPSToNGSIv2::onTerminate()
{
    // Don't need to do anything here.
}

void ISBridgeFastRTPSToNGSIv2::SubListener::onSubscriptionMatched(Subscriber* sub, MatchingInfo& info){
    if (info.status == MATCHED_MATCHING)
    {
        n_matched++;
        std::cout << "Subscriber matched" << std::endl;
        std::cout << "Sub: " << sub->getGuid() << std::endl;
        std::cout << "Topic: " << sub->getAttributes().topic.getTopicName() << std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "Subscriber unmatched" << std::endl;
    }
}

ISBridgeFastRTPSToNGSIv2::SubListener::SubListener() : user_transformation(nullptr), handle(nullptr)
{
}

ISBridgeFastRTPSToNGSIv2::SubListener::SubListener(const char* file_path) : user_transformation(nullptr), handle(nullptr)
{
    loadLibrary(file_path);
}

void ISBridgeFastRTPSToNGSIv2::SubListener::loadLibrary(const char* file_path)
{
    if(file_path){
        handle = eProsimaLoadLibrary(file_path);
        user_transformation = (transformfunc_t)eProsimaGetProcAddress(handle, "transformToNGSIv2");
        if (!user_transformation)
        {
            user_transformation = (transformfunc_t)eProsimaGetProcAddress(handle, "transform");
        }
    }
}

ISBridgeFastRTPSToNGSIv2::SubListener::~SubListener(){
    if(handle) eProsimaCloseLibrary(handle);
}

void ISBridgeFastRTPSToNGSIv2::SubListener::onNewDataMessage(Subscriber* sub){
    SerializedPayload_t serialized_input;
    JsonNGSIv2 output;
    if(sub->takeNextData(&serialized_input, &m_info)){
        if(m_info.sampleKind == ALIVE){
            if(user_transformation){
                user_transformation(&serialized_input, &output);
                listener_publisher->write(&output);
            }
            else{
                std::cout << "Error: user transformation function not defined" << std::endl;
            }
        }
    }
}

ISBridgeFastRTPSToNGSIv2::NGSIv2Publisher::NGSIv2Publisher()
{
}

ISBridgeFastRTPSToNGSIv2::NGSIv2Publisher::NGSIv2Publisher(const string host, const uint16_t port)
{
    setHostPort(host, port);
}

void ISBridgeFastRTPSToNGSIv2::NGSIv2Publisher::setHostPort(const string host, const uint16_t port)
{
    stringstream strstr;
    strstr << host << ":" << port;
    url = strstr.str();
}

ISBridgeFastRTPSToNGSIv2::NGSIv2Publisher::~NGSIv2Publisher() {}

void ISBridgeFastRTPSToNGSIv2::NGSIv2Publisher::write(JsonNGSIv2* payload)
{
    try {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        string entityId = payload->entityId();
        string data = payload->data();
        if (entityId.length() > 0) {
            // entity update
            request.setOpt(new curlpp::options::Url(url + "/v2/entities/" + entityId + "/attrs"));
        } else {
            // batch update
            request.setOpt(new curlpp::options::Url(url + "/v2/op/update"));
        }
        request.setOpt(new curlpp::options::Verbose(true));
        std::list<std::string> header;
        header.push_back("Content-Type: application/json");

        request.setOpt(new curlpp::options::HttpHeader(header));
        request.setOpt(new curlpp::options::PostFields(data));
        request.setOpt(new curlpp::options::PostFieldSize(data.length()));

        std::ostringstream response;
        request.setOpt(new curlpp::options::WriteStream(&response));

        request.perform();

        cout << "Response: " << response.str() << endl;
    }
    catch ( curlpp::LogicError & e ) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    catch ( curlpp::RuntimeError & e ) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}
