/*
* @file:  biz_actor_factory.hpp
* @brief:  actor factory to create actor
* Copyright (c) 2020 by Turingtec All Rights Reserved
*
* @author:  fanc
* @version:  v0.0.1
* @date:  2022/11/12
* @history:
*/

#ifndef __REFLECTION_FACTORY_HPP__
#define __REFLECTION_FACTORY_HPP__

#include <unordered_map>
#include <memory>
#include <functional>
#include <cxxabi.h>
#include "macro_definition.h"

NS_BEGIN

using Constructor = std::function<void *()>;

class ReflectionFactory {
public:
    static void register_actor(std::string const &className, Constructor constructor) {
        Constructors()[className] = constructor;
    }

    static void *create_actor(std::string const &className) {
        auto iter = Constructors().find(className);
        if (iter == Constructors().end()) {
            printf(">>>>>>>not find class %s Constructors\n", className.c_str());
            return nullptr;
        }
        return ((Constructor) iter->second)();
    }

    static int actor_type_count() { return ++s_actor_type_count_; }

private:
    inline static std::unordered_map<std::string, Constructor> &Constructors() {
        static std::unordered_map<std::string, Constructor> instance;
        return instance;
    }

    static inline int s_actor_type_count_{};
};

/**
* @class ActorCreator
*
* @brief A concrete ActorClass needs to inherit ActorCreator to enable reflection mechanism.
*        ActorCreator provides ``CreateFunction``, and registers ``ActorType`` and ``CreateFunction`` to
*        ActorFactory().
*
*/
template<typename T>
class ReflectionCreator {
public:
    struct Register {
        Register() {
            std::string strTypeName;
            char *szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
            if (nullptr != szDemangleName) {
                strTypeName = szDemangleName;
                free(szDemangleName);
            }
            ReflectionFactory::register_actor(strTypeName, []() {
                return new(std::nothrow) T();
            });
            printf("------------> [%d] Registered class name: %s.\n", ReflectionFactory::actor_type_count(),
                   strTypeName.c_str());
        }

        inline void do_nothing() const {}
    };

    // 生成一个具体的对象
    static std::shared_ptr<T> createObject(const std::string &name) {
        auto actor = std::shared_ptr<T>(static_cast<T *>(ReflectionFactory::create_actor(name)));
        return actor;
    }

    /**
    * @brief Constructor. A constructor to construct module creator.
    *
    * @param None.
    *
    * @return None.
    */
    ReflectionCreator() { register_.do_nothing(); }

    /**
    * @brief Destructor. A destructor to destruct module creator.
    *
    * @param None.
    *
    * @return None.
    */
    virtual ~ReflectionCreator() {}

private:
    static inline Register register_;
};

NS_END

#endif