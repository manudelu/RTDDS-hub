#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <iostream>
#include <string>

using namespace eprosima::fastdds::dds;

template <typename Derived>
class DdsSubscriber
{
public:
    explicit DdsSubscriber(TypeSupport type)
        : participant_(nullptr)
        , subscriber_(nullptr)
        , topic_(nullptr)
        , reader_(nullptr)
        , type_(std::move(type))
        , listener_(static_cast<Derived&>(*this))
    {}

    ~DdsSubscriber() { cleanup(); }

    DdsSubscriber(const DdsSubscriber&) = delete;
    DdsSubscriber& operator=(const DdsSubscriber&) = delete;

    bool has_publishers() const
    {
        return listener_.matched_count() > 0;
    }

protected: 
    bool init_dds(const std::string& topic_name,
                  const std::string& participant_name,
                  int domain_id = 0)
    {
        DomainParticipantQos pqos;
        pqos.name(participant_name);
        participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos);
        if (!participant_)
        {
            std::cerr << '[' << participant_name << "] Failed to create DDS participant\n";
            return false;
        }

        type_.register_type(participant_);

        topic_ = participant_->create_topic(topic_name, type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (!topic_)
        {
            std::cerr << '[' << participant_name << "] Failed to create topic '" << topic_name << "'\n";
            cleanup();
            return false;
        }

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (!subscriber_)
        {
            std::cerr << '[' << participant_name << "] Failed to create subscriber\n";
            cleanup();
            return false;
        }

        const DataReaderQos rqos = Derived::reader_qos();
        reader_ = subscriber_->create_datareader(topic_, rqos, &listener_);
        if (!reader_)
        {
            std::cerr << '[' << participant_name << "] Failed to create datareader\n";
            cleanup();
            return false;
        }

        return true;
    }

    void cleanup()
    {
        if (reader_)
        {
            subscriber_->delete_datareader(reader_);
            reader_ = nullptr;
        }
        if (subscriber_)
        {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }
        if (topic_)
        {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        if (participant_)
        {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }

    DomainParticipant* participant_;
    Subscriber*        subscriber_;
    Topic*             topic_;
    DataReader*        reader_;
    TypeSupport        type_;

private:
    class SubListener : public DataReaderListener
    {
    public:
        explicit SubListener(Derived& derived)
            : derived_(derived), matched_{0} 
        {}
        ~SubListener() override = default;

        void on_subscription_matched(DataReader*,
                                     const SubscriptionMatchedStatus& info) override
        {
            matched_ += info.current_count_change;
            derived_.on_matched(matched_.load(std::memory_order_relaxed));
        }

        // Called from the DDS listener thread -- derived must ensure thread safety
        // when accessing shared state in on_data_available().
        void on_data_available(DataReader* reader) override
        {
            derived_.on_data_available(reader);
        }

        int matched_count() const
        {
            return matched_.load(std::memory_order_relaxed);
        }

    private:
        Derived& derived_;
        std::atomic_int matched_;
    } listener_;
};