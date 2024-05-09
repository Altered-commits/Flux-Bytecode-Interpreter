#ifndef UNNAMED_ITERATORS_HPP
#define UNNAMED_ITERATORS_HPP

#include <string>
#include <memory>

class Iterator;

using IterPtr = std::unique_ptr<Iterator>;

class Iterator
{
    public:
        virtual const std::string& getId()      const = 0;
        virtual       Object       getCurrent() const = 0;
        virtual       void         next()             = 0;
        virtual       bool         hasNext()    const = 0;

    protected:
        std::string iterIdentifier;
};

template<typename T>
class RangeIterator : public Iterator
{
    public:
        RangeIterator(const std::string& iterId, T start, T stop, T step)
            : start(start), stop(stop), step(step)
        {
            iterIdentifier = std::move(iterId);
        }

        Object getCurrent() const override {
            return start;
        }

        const std::string& getId() const override {
            return iterIdentifier;
        }

        void next() override {
            start += step;
        }

        bool hasNext() const override {
            return step > 0 ? start < stop : start > stop;
        }

    private:
        T start, stop, step;
};

#endif