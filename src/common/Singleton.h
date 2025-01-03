//
//  Singleton.h
//  MediaFoundation
//
//  Created by liwenlong on 2021/3/1.
//  Copyright © 2021 inke. All rights reserved.
//

#ifndef Singleton_h
#define Singleton_h

#include "macro_definition.h"
#include <mutex>
#include <memory>

NS_BEGIN

template <class T>
class Singleton final{
   
public:
   static std::shared_ptr<T> instance(){
      std::call_once(once_, [&](){
          instance_.reset(new T());
      });
      return instance_;
   }
   
   Singleton(const Singleton&) = delete;//禁用copy方法
   const Singleton& operator=( const Singleton&) = delete;//禁用赋值方法
private:
   Singleton(){};
   ~Singleton(){};
   
private:
   static std::once_flag once_;
   static std::shared_ptr<T> instance_;
};

template <class T>
std::once_flag Singleton<T>::once_;

template <class T>
std::shared_ptr<T> Singleton<T>::instance_;

//Instructions for use
//auto foo = Singleton<Foo>::Instance();

NS_END
#endif /* Singleton_h */
