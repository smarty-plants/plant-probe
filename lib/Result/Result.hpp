#include <stdlib.h>

template <typename T, typename E>
class Result {
public:
    Result(const T& value, const E& error) : resultValue(value), resultError(error) {}
    Result(const T& value) : resultValue(value) {}
    Result(const E& error) : resultError(error) {}

    bool HasValue() const { return resultValue.has_value(); }
    bool HasError() const { return resultError.has_value(); }

    T& GetValue() { return resultValue.value(); }
    const T& GetValue() const { return resultValue.value(); }
    E& GetError() { return resultError.value(); }
    const E& GetError() const { return resultError.value(); }

private:
    std::optional<T> resultValue;
    std::optional<E> resultError;
};