#ifndef _JNIPP_H_
#define _JNIPP_H_ 1

// Local Dependencies
#include "internal.h"

namespace jni
{
	/**
		Initialises the Java Native Interface with the given JNIEnv handle, which
		gets passed into a native function which is called from Java. This only
		needs to be done once per process - further calls are no-ops.
		\param env A JNI environment handle.
	 */
	void init(JNIEnv* env);

	/**
		Object corresponds with a `java.lang.Object` instance. With an Object,
		you can then call Java methods, and access fields on the Object. To
		instantiate an Object of a given class, use the `Class` class.
	 */
	class Object
	{
	public:
		/** Flags which can be passed to the Object constructor. */
		enum ScopeFlags
		{
			Temporary			= 1,	///< Temporary object. Do not create a global reference.
			DeleteLocalInput	= 2		///< The input reference is temporary and can be deleted.
		};

		/** Default constructor. Creates a `null` object. */
		Object() noexcept;

		/**
			Copies a reference to another Object. Note that this is not a deep
			copy operation, and both Objects will reference the same Java
			Object.
			\param other The Object to copy.
		 */
		Object(const Object& other);

		/**
			Move constructor. Copies the Object reference from the supplied
			Object, and then nulls the supplied Object reference.
			\param other The Object to move.
		 */
		Object(Object&& other) noexcept;

		/**
			Creates an Object from a local JNI reference.
			\param ref The local JNI reference.
			\param scopeFlags Bitmask of ScopeFlags values.
		 */
		Object(jobject ref, int scopeFlags = 0);

		/** 
			Destructor. Releases this reference on the Java Object so it can be
			picked up by the garbage collector.
		 */
		virtual ~Object() noexcept;

		/**
			Assignment operator. Copies the object reference from the supplied
			Object. They will now both point to the same Java Object.
			\param other The Object to copy.
			\return This Object.
		 */
		Object& operator=(const Object& other);

		/**
			Assignment operator. Moves the object reference from the supplied
			Object to this one, and leaves the other one as a null.
			\param other The Object to move.
			\return This Object.
		 */
		Object& operator=(Object&& other);

		/**
			Tells whether this Object is currently a `null` pointer.
			\return `true` if `null`, `false` if it references an object.
		 */
		bool isNull() const noexcept;

		/**
			Gets the underlying JNI jobject handle.
			\return The JNI handle.
		 */
		jobject getHandle() const noexcept { return _handle; }

	private:
		// Instance Variables
		jobject _handle;
		mutable jclass _class;
		bool _isGlobal;
	};

	/**
		Class corresponds with `java.lang.Class`, and allows you to instantiate
		Objects and get class members such as methods and fields.
	 */
	class Class final : public Object
	{
	public:
		/**
			Obtains a class reference to the Java class with the given qualified
			name.
			\param name The qualified class name (e.g. "java/lang/String").
		 */
		Class(const char* name);

		/**
			Creates a Class object by JNI reference.
			\param ref The JNI class reference.
			\param scopeFlags Bitmask of Object::ScopeFlags.
		 */
		Class(jclass ref, int scopeFlags = 0);

		/**
			Creates a new instance of this Java class and returns a reference to
			it. The item's parameterless constructor is called.
			\return The created instance.
		 */
		Object newInstance() const;

		/**
			Creates a new instance of this Java class and returns a reference to
			it. The constructor signature is determined by the supplied parameters,
			and the parameters are then passed to the constructor.
			\param args Arguments to supply to the constructor.
			\return The created instance.
		 */
		template <class... TArgs>
		Object newInstance(TArgs... args) const {
			internal::value_t values[sizeof...(args)] = {};
			method_t constructor = getMethod("<init>", ("(" + internal::sig(args...) + ")V").c_str());
			return newObject(constructor, internal::args(values, args...));
		}

		/**
			Gets a handle to the field with the given name and type signature.
			This handle can then be stored so that the field does not need to
			be looked up by name again. It does not need to be deleted.
			\param name The name of the field.
			\param signature The JNI type signature of the field.
			\return The field ID.
		 */
		field_t getField(const char* name, const char* signature) const;

		/**
			Gets a handle to the method with the given name and signature.
			This handle can then be stored so that the method does not need
			to be looked up by name again. It does not need to be deleted.
			\param name The name of the method.
			\param signature The JNI method signature.
			\return The method ID.
		 */
		method_t getMethod(const char* name, const char* signature) const;

	private:
		// Helper Functions
		Object newObject(method_t constructor, internal::value_t* args) const;
	};

	/**
		When the application's entry point is in C++ rather than in Java, it will
		need to spin up its own instance of the Java Virtual Machine (JVM) before
		it can initialize the Java Native Interface. Vm is used to create and 
		destroy a running JVM instance.

		It uses the RAII idiom, so when the destructor is called, the Vm is shut
		down.

		Note that currently only one instance is supported. Attempts to create
		more will result in an InitializationException.
	 */
	class Vm final
	{
	public:
		/**
			Starts the Java Virtual Machine. 
			\param path The path to the jvm.dll 
		 */
		Vm(const char* path);

		/** Destroys the running instance of the JVM. */
		~Vm();
	};

	/**
		A Java method call threw an Exception.
	 */
	class InvocationException : public Exception
	{
	};

	/**
		A supplied name or type signature could not be resolved.
	 */
	class NameResolutionException : public Exception
	{
	public:
		/**
			Constructor with an error message.
			\param name The name of the unresolved symbol.
		 */
		NameResolutionException(const char* name) : Exception(name) {}
	};

	/**
		The Java Native Interface was not properly initialized.
	 */
	class InitializationException : public Exception
	{
	public:
		/**
			Constructor with an error message.
			\param msg Message to pass to the Exception.
		 */
		InitializationException(const char* msg) : Exception(msg) {}
	};
}

#endif // _JNIPP_H_

