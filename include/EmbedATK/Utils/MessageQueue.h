#pragma once

#include "EmbedATK/OSAL/OSAL.h"

namespace Utils {

    template<IsStaticPolymorphic Queue, size_t Size>
    requires (std::is_base_of_v<IPolymorphic<OSAL::MessageQueue>, Queue>)
    struct StaticMessageQueue
    {
        StaticBlockPool<Size, allocData<OSAL::MessageQueue::MsgType>()> pool;
        StaticObjectStore<OSAL::MessageQueue::MsgType*, Size> store;
        Queue queue;
    };

    template <typename T>
    struct is_static_message_queue : std::false_type {};

    template<IsStaticPolymorphic Queue, size_t Size>
    struct is_static_message_queue<Utils::StaticMessageQueue<Queue, Size>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_static_message_queue_v = is_static_message_queue<T>::value;

    template <typename T>
    concept IsStaticMessageQueue = is_static_message_queue_v<T>;

    template<IsStaticMessageQueue Queue>
    static constexpr void setupStaticMessageQueue(Queue& queue)
    {
        OSAL::createMessageQueue(queue.queue, queue.store, queue.pool);
    }

}