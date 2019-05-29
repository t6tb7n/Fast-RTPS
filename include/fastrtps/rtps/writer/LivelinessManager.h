// Copyright 2016-2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file LivelinessManager.h
 */
#ifndef LIVELINESS_MANAGER_H_
#define LIVELINESS_MANAGER_H_

#include "LivelinessData.h"
#include "../timedevent/TimedCallback.h"
#include "../../utils/collections/ResourceLimitedVector.hpp"

#include <mutex>

namespace eprosima {
namespace fastrtps {
namespace rtps {

/**
 * @brief A class managing the liveliness of a set of writers. Writers are represented by their LivelinessData
 * @details Uses a shared timed event and informs outside classes on liveliness changes
 * @ingroup WRITER_MODULE
 */
class LivelinessManager
{
public:

    /**
     * @brief Constructor
     * @param liveliness_lost_callback A callback that will be invoked when a writer loses liveliness
     * @param liveliness_lost_callback A callback that will be invoked when a writer recovers liveliness
     * @param service The asio I/O service
     * @param event_thread The event thread
     * @param manage_automatic True to manage writers with automatic liveliness, false otherwise
     */
    LivelinessManager(
            const std::function<void(GUID_t)>& liveliness_lost_callback,
            const std::function<void(GUID_t)>& liveliness_recovered_callback,
            asio::io_service& service,
            const std::thread& event_thread,
            bool manage_automatic = true);

    /**
     * @brief Constructor
     */
    ~LivelinessManager();

    /**
     * @brief LivelinessManager
     * @param other
     */
    LivelinessManager(const LivelinessManager& other) = delete;

    /**
     * @brief Adds a writer to the set
     * @param guid GUID of the writer
     * @param kind Liveliness kind
     * @param lease_duration Liveliness lease duration
     * @return True if the writer was successfully added
     */
    bool add_writer(
            GUID_t guid,
            LivelinessQosPolicyKind kind,
            Duration_t lease_duration);

    /**
     * @brief Removes a writer
     * @param guid GUID of the writer
     * @return True if the writer was successfully removed
     */
    bool remove_writer(GUID_t guid);

    /**
     * @brief Asserts liveliness of a writer in the set
     * @details Other writers are asserted as well if applicable
     * @param guid The writer to assert liveliness of
     * @return True if liveliness was successfully asserted
     */
    bool assert_liveliness(GUID_t guid);

    /**
     * @brief Asserts liveliness of writers with given liveliness kind
     * @param kind Liveliness kind
     * @return True if liveliness was successfully asserted
     */
    bool assert_liveliness(LivelinessQosPolicyKind kind);

    /**
     * @brief Asserts liveliness of writers belonging to given participant
     * @param prefix The prefix determining the participant
     * @return True if liveliness was successfully asserted
     */
    bool assert_liveliness(GuidPrefix_t prefix);

    /**
     * @brief A method to check any writer of the given kind is alive
     * @param kind The liveliness kind to check for
     * @return True if at least one writer of this kind is alive. False otherwise
     */
    bool is_any_alive(LivelinessQosPolicyKind kind);

    /**
     * @brief A method to return liveliness data
     * @details Should only be used for testing purposes
     * @return Vector of liveliness data
     */
    const ResourceLimitedVector<LivelinessData> &get_liveliness_data() const;

private:

    /**
     * @brief A method to calculate the time when the next writer is going to lose liveliness
     * @details This method is public for testing purposes but it should not be used from outside this class
     * @return True if at least one writer is alive
     */
    bool calculate_next();

    //! A method to find a writer from a guid
    //! Returns an iterator to the writer liveliness data
    //! Returns true if writer was found, false otherwise
    bool find_writer(
            GUID_t guid,
            ResourceLimitedVector<LivelinessData>::iterator* wit_out);


    //! A method called if the timer expires
    void timer_expired();

    //! A boolean indicating whether we are managing writers with automatic liveliness
    bool manage_automatic_;

    //! A vector of liveliness data
    ResourceLimitedVector<LivelinessData> writers_; // TODO raquel allocation QoS

    //! A timed callback expiring when a writer (the timer owner) loses its liveliness
    TimedCallback timer_;

    //! The timer owner, i.e. the writer which is next due to lose its liveliness
    LivelinessData* timer_owner_;

    //! A callback to inform outside classes that a writer lost its liveliness
    std::function<void(GUID_t)> liveliness_lost_callback_;

    //! A callback to inform outside classes that a writer recovered its liveliness
    std::function<void(GUID_t)> liveliness_recovered_callback_;

    //! A mutex to protect the liveliness data
    std::mutex mutex_;
};

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
#endif /* LIVELINESS_MANAGER_H_ */