#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <iostream>
#include <string>

using namespace eprosima::fastdds::dds;

template <typename Derived>
class DdsPublisher
{
public:
    explicit DdsPublisher(TypeSupport type)
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(std::move(type))
        , listener_(static_cast<Derived&>(*this))
    {}

    ~DdsPublisher() { cleanup(); }

    DdsPublisher(const DdsPublisher&) = delete;
    DdsPublisher& operator=(const DdsPublisher&) = delete;

    bool has_subscribers() const
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

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        if (!publisher_)
        {
            std::cerr << '[' << participant_name << "] Failed to create publisher\n";
            cleanup();
            return false;
        }

        // QoS is provided by the derived class!
        const DataWriterQos wqos = Derived::writer_qos();
        writer_ = publisher_->create_datawriter(topic_, wqos, &listener_);
        if (!writer_)
        {
            std::cerr << '[' << participant_name << "] Failed to create data writer\n";
            cleanup();
            return false;
        }

        return true;
    }

    void cleanup()
    {
        if (writer_)
        {
            publisher_->delete_datawriter(writer_);
            writer_ = nullptr;
        }
        if (publisher_)
        {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
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
    Publisher*         publisher_;
    Topic*             topic_;
    DataWriter*        writer_;
    TypeSupport        type_;

private:
    class PubListener : public DataWriterListener
    {
    public:
        explicit PubListener(Derived& derived) 
            : derived_(derived), matched_{0} 
        {}
        ~PubListener() override = default;

        void on_publication_matched(DataWriter*,
                                    const PublicationMatchedStatus& info) override
        {
            matched_ += info.current_count_change;
            derived_.on_matched(matched_.load(std::memory_order_relaxed));
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