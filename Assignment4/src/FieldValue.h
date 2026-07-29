//MOSTLY MY CODE
//Inspired by information provided by claude4
#pragma once
#include <string>

//concepts learned/practiced/demonstrated:
	//polymorphism
	//classes/inheritance
	//pointers
	//member initializer lists
	//enum classes


//something to represent the datatype NOT as a string (like my inner python programmer desperate wants to)
enum class DType { STRING, UINT };

//the base class
class Holder {
protected:
	void* value;
	DType dataType;

	//constructor is protected because ONLY derived classes should be able to call this constructor
	//takes the already-allocated value and its tag, so derived classes just hand it up
	Holder(void* v, DType t) : value(v), dataType(t) {}

public:
	DType getType() const {
		return dataType;
	}

	//so every derived class doesn't need to reimplement identical logic
	virtual void* get() { return value; }

	virtual ~Holder() = default;
};

//some derived classes to go along with it
class StringHolder : public Holder {
public:
	//constructor -- heap-allocate the real std::string, pass its address up to Holder
	StringHolder(const std::string& data) : Holder(new std::string(data), DType::STRING) {}

	void* get() override {
		return static_cast<std::string*>(value);
	}

	//destructor
	//requires typecast because apparently (**courtesy of claude4): void pointers cannot be directly deleted without a typecast
	~StringHolder() override {
		delete static_cast<std::string*>(value);
	}
};

class UIntHolder : public Holder {
public:
	UIntHolder(unsigned int data) : Holder(new unsigned int(data), DType::UINT) {}

	void* get() override {
		return static_cast<unsigned int*>(value);
	}

	~UIntHolder() override {
		delete static_cast<unsigned int*>(value);
	}
};

//my simple variant class (to avoid using the actual std::variant type and compiling with the std=C++17 flag)
class SimpleVariant {
public:
	Holder* container;

	SimpleVariant() : container(nullptr) {}

	//disable copying -- container is a raw owning pointer, default shallow copy would double-free
	SimpleVariant(const SimpleVariant&) = delete;
	SimpleVariant& operator=(const SimpleVariant&) = delete;

	void set(const std::string& value) {
		delete container; //free whatever was previously held (safe even if nullptr)
		container = new StringHolder(value);
	}

	void set(unsigned int value) {
		delete container;
		container = new UIntHolder(value);
	}

	//get the type (for programmer to determine which getter function to call)
	DType getType() const {
		return container->getType();
	}
	//getters must be CONST because its being referenced in a constant context but these functions still need to be called
	unsigned int getUInt() const {
		unsigned int* output = static_cast<unsigned int*>(container->get());
		return *output;
	}

	std::string getString() const {
		std::string* output = static_cast<std::string*>(container->get());
		return *output;
	}

	~SimpleVariant() {
		delete container; //virtual ~Holder() makes sure the correct derived destructor runs first
	}
};

typedef SimpleVariant FieldValue;

