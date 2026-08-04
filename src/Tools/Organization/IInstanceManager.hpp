#pragma once

#include <iostream>
#include <list>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

namespace gbe {

    // Macro to define MainType::Ref inside derived classes
#define GBE_DECLARE_INSTANCE_REF(ClassType) \
public: \
    using Ref = IInstanceManager<ClassType>::Ref;

    template <typename T>
    class IInstanceManager {
    public:
        using IdType = uint64_t;

        // Lightweight handle class used as a safe pointer alternative
        struct Ref {
            IdType id = 0;

            Ref() = default;
            Ref(IdType instanceId) : id(instanceId) {}
            Ref(const T& obj) : id(obj.getId()) {}
            Ref(const T* obj) : id(obj ? obj->getId() : 0) {}

            // Resolves handle to reference; throws if object no longer exists
            T& Get() const {
                T* ptr = IInstanceManager<T>::getById(id);
                if (!ptr) {
                    throw std::runtime_error("Attempted to access an expired or invalid IInstanceManager Ref!");
                }
                return *ptr;
            }

            // Safe non-throwing accessor
            T* GetPtr() const {
                return IInstanceManager<T>::getById(id);
            }

            bool IsValid() const {
                return IInstanceManager<T>::getById(id) != nullptr;
            }

            IdType GetID() const { return id; }

            explicit operator bool() const { return IsValid(); }
            bool operator==(const Ref& other) const { return id == other.id; }
            bool operator!=(const Ref& other) const { return id != other.id; }
        };

        IInstanceManager() {
            registerInstance();
        }

        ~IInstanceManager() {
            unregisterInstance();
        }

        IInstanceManager(const IInstanceManager&) {
            registerInstance();
        }

        IInstanceManager(IInstanceManager&&) noexcept {
            registerInstance();
        }

        // Assignment preserves existing handle identity for assigned object
        IInstanceManager& operator=(const IInstanceManager&) = default;
        IInstanceManager& operator=(IInstanceManager&&) noexcept = default;

        IdType getId() const { return m_id; }
        Ref getRef() const { return Ref(m_id); }

        // Safe lookup by unique ID
        static T* getById(IdType id) {
            auto& reg = getRegistry();
            auto it = reg.map.find(id);
            return (it != reg.map.end()) ? *(it->second) : nullptr;
        }

        static T* getOldest() {
            auto& reg = getRegistry();
            return reg.orderList.empty() ? nullptr : reg.orderList.front();
        }

        static T* getYoungest() {
            auto& reg = getRegistry();
            return reg.orderList.empty() ? nullptr : reg.orderList.back();
        }

        static size_t count() {
            return getRegistry().orderList.size();
        }

    private:
        using Container = std::list<T*>;
        using Map = std::unordered_map<IdType, typename Container::iterator>;

        struct Registry {
            Container orderList;
            Map map;
            IdType nextId = 1;
        };

        inline static Registry registry;

        IdType m_id = 0;
        typename Container::iterator m_listIter;

        void registerInstance() {
            auto& reg = getRegistry();
            m_id = reg.nextId++;
            reg.orderList.push_back(static_cast<T*>(this));
            m_listIter = std::prev(reg.orderList.end());
            reg.map[m_id] = m_listIter;
        }

        void unregisterInstance() {
            auto& reg = getRegistry();
            reg.map.erase(m_id);
            reg.orderList.erase(m_listIter);
        }

        // Encapsulates storage to prevent static initialization ordering bugs
        static Registry& getRegistry() {
            return registry;
        }
    };
}