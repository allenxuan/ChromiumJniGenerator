//
// Created by Xuanyi Huang on 1/20/22.
//

#ifndef ANDROID_CHROMIUM_JNI_GENERATOR_NATIVE_ESSENTIAL_H
#define ANDROID_CHROMIUM_JNI_GENERATOR_NATIVE_ESSENTIAL_H

//START
#include <jni.h>
#include <assert.h>
#define JNI_REGISTRATION_EXPORT __attribute__((visibility("default")))
#define BASE_EXPORT __attribute__((visibility("default")))
#define DDCHECK(class) assert(class)
#define CHECK_CLAZZ(env, jcaller, clazz, ...) DDCHECK(clazz);
#define DDCHECK_EQ(param1, param2) assert(param1 == param2)
#include <atomic>
#include <memory>
#include <sys/prctl.h>

#if defined(COMPILER_GCC) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#elif defined(COMPILER_MSVC)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif

#if defined(COMPILER_GCC) && defined(NDEBUG)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#if defined(ARCH_CPU_X86)
// Dalvik JIT generated code doesn't guarantee 16-byte stack alignment on
// x86 - use force_align_arg_pointer to realign the stack at the JNI
// boundary. crbug.com/655248
#define JNI_GENERATOR_EXPORT \
  extern "C" __attribute__((visibility("default"), force_align_arg_pointer))
#else
#define JNI_GENERATOR_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// This class is a wrapper for JNIEnv Get(Static)MethodID.
class BASE_EXPORT MethodID {
 public:
  enum Type {
    TYPE_STATIC,
    TYPE_INSTANCE,
  };

  // Returns the method ID for the method with the specified name and signature.
  // This method triggers a fatal assertion if the method could not be found.
  template<Type type>
  static jmethodID Get(JNIEnv* env,
                       jclass clazz,
                       const char* method_name,
                       const char* jni_signature);

  // The caller is responsible to zero-initialize |atomic_method_id|.
  // It's fine to simultaneously call this on multiple threads referencing the
  // same |atomic_method_id|.
  template<Type type>
  static jmethodID LazyGet(JNIEnv* env,
                           jclass clazz,
                           const char* method_name,
                           const char* jni_signature,
                           std::atomic<jmethodID>* atomic_method_id);
};

// A 32 bit number could be an address on stack. Random 64 bit marker on the
// stack is much less likely to be present on stack.
constexpr uint64_t kJniStackMarkerValue = 0xbdbdef1bebcade1b;

// Context about the JNI call with exception checked to be stored in stack.
struct BASE_EXPORT JniJavaCallContextUnchecked {
  ALWAYS_INLINE JniJavaCallContextUnchecked() {
      // TODO(ssid): Implement for other architectures.
#if defined(__arm__) || defined(__aarch64__)
      // This assumes that this method does not increment the stack pointer.
            asm volatile("mov %0, sp" : "=r"(sp));
#else
      sp = 0;
#endif
  }

  // Force no inline to reduce code size.
  template <MethodID::Type type>
  NOINLINE void Init(JNIEnv* env,
                     jclass clazz,
                     const char* method_name,
                     const char* jni_signature,
                     std::atomic<jmethodID>* atomic_method_id) {
      env1 = env;

      // Make sure compiler doesn't optimize out the assignment.
      memcpy(&marker, &kJniStackMarkerValue, sizeof(kJniStackMarkerValue));
      // Gets PC of the calling function.
      pc = reinterpret_cast<uintptr_t>(__builtin_return_address(0));

      method_id = MethodID::LazyGet<type>(
              env, clazz, method_name, jni_signature, atomic_method_id);
  }

  NOINLINE ~JniJavaCallContextUnchecked() {
      // Reset so that spurious marker finds are avoided.
      memset(&marker, 0, sizeof(marker));
  }

  uint64_t marker;
  uintptr_t sp;
  uintptr_t pc;

  // raw_ptr<JNIEnv> env1;
  JNIEnv* env1;
  jmethodID method_id;
};

// If |atomic_method_id| set, it'll return immediately. Otherwise, it'll call
// into ::Get() above. If there's a race, it's ok since the values are the same
// (and the duplicated effort will happen only once).
template <MethodID::Type type>
jmethodID MethodID::LazyGet(JNIEnv* env,
                            jclass clazz,
                            const char* method_name,
                            const char* jni_signature,
                            std::atomic<jmethodID>* atomic_method_id) {
    const jmethodID value = atomic_method_id->load(std::memory_order_acquire);
    if (value)
        return value;
    jmethodID id = MethodID::Get<type>(env, clazz, method_name, jni_signature);
    atomic_method_id->store(id, std::memory_order_release);
    return id;
}

template<MethodID::Type type>
jmethodID MethodID::Get(JNIEnv* env,
                        jclass clazz,
                        const char* method_name,
                        const char* jni_signature) {
    auto get_method_ptr = type == MethodID::TYPE_STATIC ?
                          &JNIEnv::GetStaticMethodID :
                          &JNIEnv::GetMethodID;
    jmethodID id = (env->*get_method_ptr)(clazz, method_name, jni_signature);
    // if (base::android::ClearException(env) || !id) {
    //     LOG(FATAL) << "Failed to find " <<
    //                (type == TYPE_STATIC ? "static " : "") <<
    //                "method " << method_name << " " << jni_signature;
    // }
    return id;
}


// Context about the JNI call with exception unchecked to be stored in stack.
struct BASE_EXPORT JniJavaCallContextChecked {
  // Force no inline to reduce code size.
  template <MethodID::Type type>
  NOINLINE void Init(JNIEnv* env,
                     jclass clazz,
                     const char* method_name,
                     const char* jni_signature,
                     std::atomic<jmethodID>* atomic_method_id) {
      base.Init<type>(env, clazz, method_name, jni_signature, atomic_method_id);
      // Reset |pc| to correct caller.
      base.pc = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
  }

  NOINLINE ~JniJavaCallContextChecked() {
      // jni_generator::CheckException(base.env1);
  }

  JniJavaCallContextUnchecked base;
};

// Forward declare the generic java reference template class.
template <typename T>
class JavaRef;

// Template specialization of JavaRef, which acts as the base class for all
// other JavaRef<> template types. This allows you to e.g. pass
// ScopedJavaLocalRef<jstring> into a function taking const JavaRef<jobject>&
template <>
class BASE_EXPORT JavaRef<jobject> {
 public:
  // Initializes a null reference.
  constexpr JavaRef() {}

  // Allow nullptr to be converted to JavaRef. This avoids having to declare an
  // empty JavaRef just to pass null to a function, and makes C++ "nullptr" and
  // Java "null" equivalent.
  constexpr JavaRef(std::nullptr_t) {}

  JavaRef(const JavaRef&) = delete;
  JavaRef& operator=(const JavaRef&) = delete;

  // Public to allow destruction of null JavaRef objects.
  ~JavaRef() {}

  // TODO(torne): maybe rename this to get() for consistency with unique_ptr
  // once there's fewer unnecessary uses of it in the codebase.
  jobject obj() const { return obj_; }

  explicit operator bool() const { return obj_ != nullptr; }

  // Deprecated. Just use bool conversion.
  // TODO(torne): replace usage and remove this.
  bool is_null() const { return obj_ == nullptr; }

 protected:
  // Takes ownership of the |obj| reference passed; requires it to be a local
  // reference type.
  // #if DCHECK_IS_ON()
  //         // Implementation contains a DCHECK; implement out-of-line when DCHECK_IS_ON.
  //   JavaRef(JNIEnv* env, jobject obj);
  // #else
  //         JavaRef(JNIEnv* env, jobject obj) : obj_(obj) {}
  // #endif
  JavaRef(JNIEnv* env, jobject obj);

  // Used for move semantics. obj_ must have been released first if non-null.
  void steal(JavaRef&& other) {
      obj_ = other.obj_;
      other.obj_ = nullptr;
  }

  // The following are implementation detail convenience methods, for
  // use by the sub-classes.
  JNIEnv* SetNewLocalRef(JNIEnv* env, jobject obj);
  void SetNewGlobalRef(JNIEnv* env, jobject obj);
  void ResetLocalRef(JNIEnv* env);
  void ResetGlobalRef();
  jobject ReleaseInternal();

 private:
  jobject obj_ = nullptr;
};

// Generic base class for ScopedJavaLocalRef and ScopedJavaGlobalRef. Useful
// for allowing functions to accept a reference without having to mandate
// whether it is a local or global type.
template <typename T>
class JavaRef : public JavaRef<jobject> {
 public:
  constexpr JavaRef() {}
  constexpr JavaRef(std::nullptr_t) {}

  JavaRef(const JavaRef&) = delete;
  JavaRef& operator=(const JavaRef&) = delete;

  ~JavaRef() {}

  T obj() const { return static_cast<T>(JavaRef<jobject>::obj()); }

  //   // Get a JavaObjectArrayReader for the array pointed to by this reference.
  //   // Only defined for JavaRef<jobjectArray>.
  //   // You must pass the type of the array elements (usually jobject) as the
  //   // template parameter.
  //   template <typename ElementType,
  //             typename T_ = T,
  //             typename = std::enable_if_t<std::is_same<T_, jobjectArray>::value>>
  //   JavaObjectArrayReader<ElementType> ReadElements() const {
  //     return JavaObjectArrayReader<ElementType>(*this);
  //   }

 protected:
  JavaRef(JNIEnv* env, T obj) : JavaRef<jobject>(env, obj) {}
};

// Holds a global reference to a Java object. The global reference is scoped
// to the lifetime of this object. This class does not hold onto any JNIEnv*
// passed to it, hence it is safe to use across threads (within the constraints
// imposed by the underlying Java object that it references).
template <typename T>
class ScopedJavaGlobalRef : public JavaRef<T> {
 public:
  constexpr ScopedJavaGlobalRef() {}
  constexpr ScopedJavaGlobalRef(std::nullptr_t) {}

  // Copy constructor. This is required in addition to the copy conversion
  // constructor below.
  ScopedJavaGlobalRef(const ScopedJavaGlobalRef& other) { Reset(other); }

  // Copy conversion constructor.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaGlobalRef(const ScopedJavaGlobalRef<U>& other) {
      Reset(other);
  }

  // Move constructor. This is required in addition to the move conversion
  // constructor below.
  ScopedJavaGlobalRef(ScopedJavaGlobalRef&& other) {
      JavaRef<T>::steal(std::move(other));
  }

  // Move conversion constructor.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaGlobalRef(ScopedJavaGlobalRef<U>&& other) {
      JavaRef<T>::steal(std::move(other));
  }

  // Conversion constructor for other JavaRef types.
  explicit ScopedJavaGlobalRef(const JavaRef<T>& other) { Reset(other); }

  // Create a new global reference to the object.
  // Deprecated. Don't use bare jobjects; use a JavaRef as the input.
  ScopedJavaGlobalRef(JNIEnv* env, T obj) { Reset(env, obj); }

  ~ScopedJavaGlobalRef() { Reset(); }

  // Null assignment, for disambiguation.
  ScopedJavaGlobalRef& operator=(std::nullptr_t) {
      Reset();
      return *this;
  }

  // Copy assignment.
  ScopedJavaGlobalRef& operator=(const ScopedJavaGlobalRef& other) {
      Reset(other);
      return *this;
  }

  // Copy conversion assignment.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaGlobalRef& operator=(const ScopedJavaGlobalRef<U>& other) {
      Reset(other);
      return *this;
  }

  // Move assignment.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaGlobalRef& operator=(ScopedJavaGlobalRef<U>&& other) {
      Reset();
      JavaRef<T>::steal(std::move(other));
      return *this;
  }

  // Assignment for other JavaRef types.
  ScopedJavaGlobalRef& operator=(const JavaRef<T>& other) {
      Reset(other);
      return *this;
  }

  void Reset() { JavaRef<T>::ResetGlobalRef(); }

  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  void Reset(const ScopedJavaGlobalRef<U>& other) {
      Reset(nullptr, other.obj());
  }

  void Reset(const JavaRef<T>& other) { Reset(nullptr, other.obj()); }

  // Deprecated. You can just use Reset(const JavaRef&).
  // void Reset(JNIEnv* env, const JavaParamRef<T>& other) {
  //     Reset(env, other.obj());
  // }

  // Deprecated. Don't use bare jobjects; use a JavaRef as the input.
  void Reset(JNIEnv* env, T obj) { JavaRef<T>::SetNewGlobalRef(env, obj); }

  // Releases the global reference to the caller. The caller *must* delete the
  // global reference when it is done with it. Note that calling a Java method
  // is *not* a transfer of ownership and Release() should not be used.
  T Release() { return static_cast<T>(JavaRef<T>::ReleaseInternal()); }
};

// Holds a local reference to a Java object. The local reference is scoped
// to the lifetime of this object.
// Instances of this class may hold onto any JNIEnv passed into it until
// destroyed. Therefore, since a JNIEnv is only suitable for use on a single
// thread, objects of this class must be created, used, and destroyed, on a
// single thread.
// Therefore, this class should only be used as a stack-based object and from a
// single thread. If you wish to have the reference outlive the current
// callstack (e.g. as a class member) or you wish to pass it across threads,
// use a ScopedJavaGlobalRef instead.
template <typename T>
class ScopedJavaLocalRef : public JavaRef<T> {
 public:
  // Take ownership of a bare jobject. This does not create a new reference.
  // This should only be used by JNI helper functions, or in cases where code
  // must call JNIEnv methods directly.
  static ScopedJavaLocalRef Adopt(JNIEnv* env, T obj) {
      return ScopedJavaLocalRef(env, obj);
  }

  constexpr ScopedJavaLocalRef() {}
  constexpr ScopedJavaLocalRef(std::nullptr_t) {}

  // Copy constructor. This is required in addition to the copy conversion
  // constructor below.
  ScopedJavaLocalRef(const ScopedJavaLocalRef& other) : env_(other.env_) {
      JavaRef<T>::SetNewLocalRef(env_, other.obj());
  }

  // Copy conversion constructor.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaLocalRef(const ScopedJavaLocalRef<U>& other) : env_(other.env_) {
      JavaRef<T>::SetNewLocalRef(env_, other.obj());
  }

  // Move constructor. This is required in addition to the move conversion
  // constructor below.
  ScopedJavaLocalRef(ScopedJavaLocalRef&& other) : env_(other.env_) {
      JavaRef<T>::steal(std::move(other));
  }

  // Move conversion constructor.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaLocalRef(ScopedJavaLocalRef<U>&& other) : env_(other.env_) {
      JavaRef<T>::steal(std::move(other));
  }

  // Constructor for other JavaRef types.
  explicit ScopedJavaLocalRef(const JavaRef<T>& other) { Reset(other); }

  // Assumes that |obj| is a local reference to a Java object and takes
  // ownership of this local reference.
  // TODO(torne): make legitimate uses call Adopt() instead, and make this
  // private.
  ScopedJavaLocalRef(JNIEnv* env, T obj) : JavaRef<T>(env, obj), env_(env) {}

  ~ScopedJavaLocalRef() { Reset(); }

  // Null assignment, for disambiguation.
  ScopedJavaLocalRef& operator=(std::nullptr_t) {
      Reset();
      return *this;
  }

  // Copy assignment.
  ScopedJavaLocalRef& operator=(const ScopedJavaLocalRef& other) {
      Reset(other);
      return *this;
  }

  // Copy conversion assignment.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaLocalRef& operator=(const ScopedJavaLocalRef<U>& other) {
      Reset(other);
      return *this;
  }

  // Move assignment.
  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  ScopedJavaLocalRef& operator=(ScopedJavaLocalRef<U>&& other) {
      env_ = other.env_;
      Reset();
      JavaRef<T>::steal(std::move(other));
      return *this;
  }

  // Assignment for other JavaRef types.
  ScopedJavaLocalRef& operator=(const JavaRef<T>& other) {
      Reset(other);
      return *this;
  }

  void Reset() { JavaRef<T>::ResetLocalRef(env_); }

  template <typename U,
          typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  void Reset(const ScopedJavaLocalRef<U>& other) {
      // We can copy over env_ here as |other| instance must be from the same
      // thread as |this| local ref. (See class comment for multi-threading
      // limitations, and alternatives).
      env_ = JavaRef<T>::SetNewLocalRef(other.env_, other.obj());
  }

  void Reset(const JavaRef<T>& other) {
      // If |env_| was not yet set (is still null) it will be attached to the
      // current thread in SetNewLocalRef().
      env_ = JavaRef<T>::SetNewLocalRef(env_, other.obj());
  }

  // Releases the local reference to the caller. The caller *must* delete the
  // local reference when it is done with it. Note that calling a Java method
  // is *not* a transfer of ownership and Release() should not be used.
  T Release() { return static_cast<T>(JavaRef<T>::ReleaseInternal()); }

 private:
  // This class is only good for use on the thread it was created on so
  // it's safe to cache the non-threadsafe JNIEnv* inside this object.
  JNIEnv* env_ = nullptr;

  // Prevent ScopedJavaLocalRef(JNIEnv*, T obj) from being used to take
  // ownership of a JavaParamRef's underlying object - parameters are not
  // allowed to be deleted and so should not be owned by ScopedJavaLocalRef.
  // TODO(torne): this can be removed once JavaParamRef no longer has an
  // implicit conversion back to T.
  // ScopedJavaLocalRef(JNIEnv* env, const JavaParamRef<T>& other);

  // Friend required to get env_ from conversions.
  template <typename U>
  friend class ScopedJavaLocalRef;

  // Avoids JavaObjectArrayReader having to accept and store its own env.
  template <typename U>
  friend class JavaObjectArrayReader;
};

ScopedJavaLocalRef<jclass> GetClassInternal(JNIEnv* env,
                                            const char* class_name,
                                            jobject class_loader);

ScopedJavaLocalRef<jclass> GetClass(JNIEnv* env, const char* class_name);

// This is duplicated with LazyGetClass above because these are performance
// sensitive.
jclass LazyGetClass(JNIEnv* env,
                    const char* class_name,
                    std::atomic<jclass>* atomic_class_id);

// Initializes the global JVM.
BASE_EXPORT void InitVM(JavaVM* vm);

// Detaches the current thread from VM if it is attached.
BASE_EXPORT void DetachFromVM();

// Returns true if the global JVM has been initialized.
BASE_EXPORT bool IsVMInitialized();


JNIEnv* AttachCurrentThread();
//END

#endif //ANDROID_CHROMIUM_JNI_GENERATOR_NATIVE_ESSENTIAL_H
