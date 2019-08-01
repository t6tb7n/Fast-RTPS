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

/**
 * @file HelloWorldSubscriber.cpp
 *
 */

#include "HelloWorldSubscriber.h"
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastrtps/utils/eClock.h>

#include <fastrtps/types/DynamicDataFactory.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

HelloWorldSubscriber::HelloWorldSubscriber():mp_participant(nullptr),
    mp_subscriber(nullptr), m_listener(this)
{
}

bool HelloWorldSubscriber::init()
{
    ParticipantAttributes PParam;
    PParam.rtps.builtin.discovery_config.discoveryProtocol = SIMPLE;
    PParam.rtps.builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    PParam.rtps.builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    PParam.rtps.builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    PParam.rtps.builtin.domainId = 0;
    PParam.rtps.builtin.discovery_config.leaseDuration = c_TimeInfinite;
    PParam.rtps.setName("Participant_sub");
    mp_participant = DomainParticipantFactory::get_instance()->create_participant(PParam, &m_listener);
    if(mp_participant==nullptr)
        return false;

    // CREATE THE COMMON READER ATTRIBUTES
    qos_.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    qos_.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
    topic_.topicKind = NO_KEY;
    topic_.topicDataType = "HelloWorld";
    topic_.topicName = "HelloWorldTopic";
    topic_.historyQos.kind = KEEP_LAST_HISTORY_QOS;
    topic_.historyQos.depth = 30;
    topic_.resourceLimitsQos.max_samples = 50;
    topic_.resourceLimitsQos.allocated_samples = 20;

    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber() {
    DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
    readers_.clear();
    datas_.clear();
}

void HelloWorldSubscriber::SubListener::on_subscription_matched(
        eprosima::fastdds::dds::DataReader*,
        eprosima::fastrtps::rtps::MatchingInfo& info)
{
    if(info.status == MATCHED_MATCHING)
    {
        n_matched++;
        std::cout << "Subscriber matched"<<std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "Subscriber unmatched"<<std::endl;
    }
}

void HelloWorldSubscriber::SubListener::on_data_available(eprosima::fastdds::dds::DataReader* reader)
{
    auto dit = subscriber_->datas_.find(reader);

    if (dit != subscriber_->datas_.end())
    {
        types::DynamicData_ptr data = dit->second;
        if (reader->take_next_sample(data.get(), &m_info))
        {
            if(m_info.sampleKind == ALIVE)
            {
                types::DynamicType_ptr type = subscriber_->readers_[reader];
                this->n_samples++;
                subscriber_->print_dynamic_data(data, type);
            }
        }
    }
}

void HelloWorldSubscriber::SubListener::on_type_discovery(
        DomainParticipant*,
        const string_255& topic,
        const types::TypeIdentifier*,
        const types::TypeObject*,
        types::DynamicType_ptr dyn_type)
{
    TypeSupport m_type(new types::DynamicPubSubType(dyn_type));
    subscriber_->mp_participant->register_type(m_type);

    std::cout << "Discovered type: " << m_type->getName() << " from topic " << topic << std::endl;

    if (subscriber_->mp_subscriber == nullptr)
    {
        SubscriberAttributes Rparam = subscriber_->att_;
        Rparam.topic = subscriber_->topic_;
        Rparam.topic.topicName = topic;
        Rparam.qos = subscriber_->qos_;
        subscriber_->mp_subscriber = subscriber_->mp_participant->create_subscriber(
            SUBSCRIBER_QOS_DEFAULT, Rparam, nullptr);

        if (subscriber_->mp_subscriber == nullptr)
        {
            return;
        }
    }
    subscriber_->topic_.topicDataType = m_type->getName();
    DataReader* reader = subscriber_->mp_subscriber->create_datareader(
        subscriber_->topic_,
        subscriber_->qos_,
        &subscriber_->m_listener);

    subscriber_->readers_[reader] = dyn_type;
    types::DynamicData_ptr data = types::DynamicDataFactory::get_instance()->create_data(dyn_type);
    subscriber_->datas_[reader] = data;
}

void HelloWorldSubscriber::print_dynamic_data(
        types::DynamicData_ptr data,
        types::DynamicType_ptr type) const
{
    std::cout << "Received data of type " << type->get_name() << std::endl;
    switch(type->get_kind())
    {
        case types::TK_STRUCTURE:
        {
            std::map<types::MemberId, types::DynamicTypeMember*> members;
            type->get_all_members(members);
            for (auto it : members)
            {
                print_member_data(data, it.second);
            }
            break;
        }
        default:
        {
            std::cout << "Only structs are supported as base type." << std::endl;
        }
    }
}

void HelloWorldSubscriber::print_member_data(
        types::DynamicData_ptr data,
        types::DynamicTypeMember* type,
        const std::string& tab) const
{
    using namespace types;
    std::cout << tab << type->get_name() << ": ";
    const MemberDescriptor* desc = type->get_descriptor();
    switch(desc->get_kind())
    {
        case TK_NONE:
        {
            std::cout << " <type not defined!>" << std::endl;
            break;
        }
        case TK_BOOLEAN:
        {
            std::cout << (data->get_bool_value(type->get_id()) ? "true" : "false") << std::endl;
            break;
        }
        case TK_BYTE:
        {
            std::cout << data->get_byte_value(type->get_id()) << std::endl;
            break;
        }
        case TK_INT16:
        {
            std::cout << data->get_int16_value(type->get_id()) << std::endl;
            break;
        }
        case TK_INT32:
        {
            std::cout << data->get_int32_value(type->get_id()) << std::endl;
            break;
        }
        case TK_INT64:
        {
            std::cout << data->get_int64_value(type->get_id()) << std::endl;
            break;
        }
        case TK_UINT16:
        {
            std::cout << data->get_uint16_value(type->get_id()) << std::endl;
            break;
        }
        case TK_UINT32:
        {
            std::cout << data->get_uint32_value(type->get_id()) << std::endl;
            break;
        }
        case TK_UINT64:
        {
            std::cout << data->get_uint64_value(type->get_id()) << std::endl;
            break;
        }
        case TK_FLOAT32:
        {
            std::cout << data->get_float32_value(type->get_id()) << std::endl;
            break;
        }
        case TK_FLOAT64:
        {
            std::cout << data->get_float64_value(type->get_id()) << std::endl;
            break;
        }
        case TK_FLOAT128:
        {
            std::cout << data->get_float128_value(type->get_id()) << std::endl;
            break;
        }
        case TK_CHAR8:
        {
            std::cout << data->get_char8_value(type->get_id()) << std::endl;
            break;
        }
        case TK_CHAR16:
        {
            std::cout << data->get_char16_value(type->get_id()) << std::endl;
            break;
        }
        case TK_STRING8:
        {
            std::cout << data->get_string_value(type->get_id()) << std::endl;
            break;
        }
        case TK_STRING16:
        {
            std::cout << "wstring print not supported in the example, use string instead." << std::endl;
            break;
        }
        case TK_STRUCTURE:
        {
            DynamicData* st_data = data->loan_value(type->get_id());
            std::map<types::MemberId, types::DynamicTypeMember*> members;
            desc->get_type()->get_all_members(members);
            for (auto it : members)
            {
                print_member_data(st_data, it.second, tab + "\t");
            }
            data->return_loaned_value(st_data);
            break;
        }
        case TK_UNION:
        {
            std::cout << "union print not supported in the example." << std::endl;
            break;
        }
        case TK_SEQUENCE:
        {
            std::cout << "sequence print not supported in the example." << std::endl;
            break;
        }
        case TK_ARRAY:
        {
            std::cout << "array print not supported in the example." << std::endl;
            break;
        }
        default:
            break;
    }
}

void HelloWorldSubscriber::run()
{
    std::cout << "Subscriber running. Please press enter to stop the Subscriber" << std::endl;
    std::cin.ignore();
}

void HelloWorldSubscriber::run(uint32_t number)
{
    std::cout << "Subscriber running until "<< number << "samples have been received"<<std::endl;
    while(number > this->m_listener.n_samples)
        eClock::my_sleep(500);
}