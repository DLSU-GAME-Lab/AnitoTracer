namespace gbe {
#define SINGLETON_MACRO_CUSTOM(Type) \
public:\
    static Type& GetInstance() { \
        static Type instance; \
        return instance; \
    } \
private:\
    Type(); \
    Type& operator=(const Type&) = delete; \
    Type(const Type&) = delete;
#define SINGLETON_MACRO_DEFAULT(Type) \
public:\
    static Type& GetInstance() { \
        static Type instance; \
        return instance; \
    } \
private:\
    Type() = default; \
    Type& operator=(const Type&) = delete; \
    Type(const Type&) = delete;
}