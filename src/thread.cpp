#include "thread.h"

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace tp {

size_t thread::hardware_concurrency() {
    std::ifstream cpuinfo("/proc/cpuinfo");

    return std::count(std::istream_iterator<std::string>(cpuinfo),
                      std::istream_iterator<std::string>(),
                      std::string("processor"));
}

thread::thread(thread &&other) noexcept
    : handle_(std::move(other.handle_))
    , id_(std::move(other.id_))
    , thread_params_(std::move(other.thread_params_)) {
    other.handle_.reset();
    other.id_.reset();
}

thread &thread::operator=(thread &&other) noexcept {
    handle_ = std::move(other.handle_);
    id_ = std::move(other.id_);
    thread_params_ = std::move(other.thread_params_);
    other.handle_.reset();
    other.id_.reset();
    return *this;
}

thread::~thread() noexcept {
    if (joinable()) {
        std::terminate();
    }
}

bool thread::joinable() const noexcept {
    return handle_.has_value();
}

size_t thread::get_id() const {
    return id_.value();
}

size_t thread::native_handle() const {
    return handle_.value();
}

void thread::join() noexcept {
    if (!joinable()) {
        return;
    }

    const auto error = pthread_join(handle_.value(), nullptr);
    if (print_errors_) {
        switch (error) {
        case EDEADLK:
            std::cout << "A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread." << std::endl;
            break;
        case EINVAL:
            std::cout << "Thread is not a joinable thread, or another thread is already waiting to join with this thread." << std::endl;
            break;
        case ESRCH:
            std::cout <<  "No thread with the ID thread could be found." << std::endl;
            break;
        }
    }

    handle_.reset();
    id_.reset();
}

void thread::detach() noexcept {
    if (!joinable()) {
        return;
    }

    const auto error = pthread_detach(handle_.value());
    if (print_errors_) {
        switch (error) {
        case EINVAL:
            std::cout << "This thread is not joinable" << std::endl;
            break;
        case ESRCH:
            std::cout << "Thread ID not found" << std::endl;
            break;
        }
    }

    handle_.reset();
    id_.reset();
}

void thread::create(const Params &params) noexcept {
    pthread_attr_t attributes{};
    pthread_attr_init(&attributes);
    if (params.stack_size.has_value()) {
        pthread_attr_setstacksize(&attributes, params.stack_size.value());
    }

    thread_params_->store_id_callback = [this](const size_t id) { id_ = id; };

    pthread_t handle{};

    const auto error = pthread_create(&handle, &attributes, &thread::thread_wrapper, thread_params_.release());
    if (print_errors_) {
        switch (error) {
        case EAGAIN:
            std::cout << "Insufficient resources to create another thread, or a system-imposed limit on the number of threads was encountered." << std::endl;
            return;
        case EINVAL:
            std::cout << "Invalid settings in attr." << std::endl;
            return;
        case EPERM:
            std::cout << "No permission to set the scheduling policy and parameters specified in attr." << std::endl;
            return;
        default:
            break;
        }
    }

    if (0 == error) {
        handle_ = static_cast<size_t>(handle);
    }
}

void *thread::thread_wrapper(void *args) {
    const auto thread_id = syscall(__NR_gettid);
    auto params = std::unique_ptr<ThreadParams>(static_cast<ThreadParams *>(args));

    params->store_id_callback(thread_id);
    params->run_callback();

    return nullptr;
}

} // namespace tp
