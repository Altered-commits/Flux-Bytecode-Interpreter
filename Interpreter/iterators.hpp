#ifndef UNNAMED_ITERATORS_HPP
#define UNNAMED_ITERATORS_HPP

#include <string>
#include <memory>

class Iterator;
//Mark this as extern to get access to globalStack residing in interpreter.cpp
extern std::vector<Object> globalStack;

using IterPtr = std::unique_ptr<Iterator>;

class Iterator
{
    public:
        virtual const std::string& getId()      const = 0;
        virtual       Object       getCurrent() const = 0;
        virtual       void         recalcStep()       = 0;
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

        //Lets keep it simple for now
        void recalcStep() override {
            step = start < stop ? 1 : -1;
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

//Reverse iteration, going from start to end is reverse apparently and idk how that happened, so we go from end to start
class EllipsisIterator : public Iterator
{
public:
    EllipsisIterator(const std::string& iterId, std::size_t start, std::size_t size)
        : start(start), end(start + size - 1)
    {
        iterIdentifier = std::move(iterId);
    }

    Object getCurrent() const override {
        return globalStack[end];
    }

    const std::string& getId() const override {
        return iterIdentifier;
    }

    //No need for this
    void recalcStep() override {}

    void next() override {
        end -= 1;
    }

    bool hasNext() const override {
        return end >= start;
    }

private:
    std::size_t start, end;
};

#endif