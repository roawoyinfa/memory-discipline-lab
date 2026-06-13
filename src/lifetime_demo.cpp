#include <iostream>

// A class type that announces its construction and destruction
class Tracer {
public:
    Tracer() : id_{nextId++} {
        std::cout << "  [TRACER " << id_ << "] CONSTRUCTED at " << this << '\n';
    }
    ~Tracer() {
        std::cout << "  [TRACER " << id_ << "] DESTROYED at " << this << '\n';
    }
    int getId() const { return this->id_; }

private:
    inline static int nextId = 0;
    int id_;
};

void demonstrateStackLifetime() {
    std::cout << "\n=== STACK LIFETIME ===\n";

    // FUNCTION BLOCK ENTERED
    //
    // The function activation record (stack frame) is established. Storage for
    // automatic objects in this invocation is reserved. At this point, no
    // object has been constructed yet.
    //
    // Storage for 'a' has been reserved, but no Tracer object exists in that
    // storage yet.

    Tracer a;
    // LIFETIME BEGINS ('a'):
    //
    // Tracer::Tracer() is invoked and constructs the object in the storage
    // reserved for 'a'. Once construction completes, the object lifetime begins
    // and a live Tracer object occupies that storage.

    {
        std::cout << "  Entering inner scope\n";

        // INNER BLOCK ENTERED
        //
        // Storage for automatic objects in this scope is available.
        // (Typical implementations often reserve storage for all local
        // automatic variables as part of the function's stack frame at
        // function entry, regardless of nesting, but this is an
        // implementation-detail not a standard guarantee. The standard
        // guarantees that suitable storage for the object exists before its
        // lifetime begins.)
        //
        // Storage for 'b' has been reserved, but no Tracer object exists yet.

        Tracer b;
        // LIFETIME BEGINS ('b'):
        //
        // Tracer::Tracer() constructs the object in the storage reserved for
        // 'b'. Once construction completes, the object lifetime begins and live
        // Tracer object occupies that storage.

        std::cout << "  Leaving inner scope\n";

    } // LIFETIME ENDS ('b'):
      //
      // Tracer::~Tracer() is invoked and the Tracer object is destroyed.
      //
      // STORAGE RELEASED ('b'):
      //
      // Any storage associated with this block is no longer reserved for
      // live objects.

    // 'a' still exists here.

} // LIFETIME ENDS ('a'):
  //
  // Tracer::~Tracer() is invoked and the Tracer object is destroyed.
  //
  // STORAGE RELEASED:
  //
  // The function returns and its activation record is removed or reused.
  // The storage previously associated with this invocation's automatic
  // objects is no longer reserved for them.

void demonstrateHeapLifetime() {
    std::cout << "\n=== HEAP LIFETIME ===\n";

    // FUNCTION BLOCK ENTERED
    //
    // Storage for the pointer variable 'a' is reserved in the function's
    // activation record (or optimized register), but 'a' and the Tracer object
    // it points to do not exist yet.
    //
    // The pointer variable 'a' and the Tracer object it points to are distinct
    // objects with independent storage duration and lifetime.

    Tracer *a{new Tracer};
    // STORAGE ACQUIRED (Tracer object):
    //
    // The new expression is executed. It requests storage for a Tracer object
    // from the dynamic storage allocator, and returns a pointer to the
    // allocated storage.
    //
    // LIFETIME BEGINS (Tracer object):
    //
    // Tracer::Tracer() is invoked and constructs the object in the dynamically
    // allocated storage. Once construction completes, the object lifetime
    // begins and a live Tracer object occupies the dynamically allocated
    // storage.
    //
    // LIFETIME BEGINS ('a'):
    //
    // The pointer variable 'a' is initialized from the address returned by the
    // new expression. Once initialization completes, a live pointer object
    // occupies its automatic storage.

    // IMPORTANT:
    //
    // unlike an automatic object, the lifetime of a dynamically allocated
    // object is not tied to any scope. Leaving the block doesn't destroy the
    // object. The object remains alive until a matching delete expression
    // destroys it and releases its storage.
    //
    // The programmer is responsible for ensuring that delete is eventually
    // performed.

    delete a;
    // LIFETIME ENDS (Tracer object):
    //
    // Tracer::~Tracer() is invoked and the Tracer object is destroyed.
    //
    // STORAGE RELEASED (Tracer object):
    //
    // The allocated storage is returned back to the allocator.

} // LIFETIME ENDS ('a'):
  //
  // The pointer object is destroyed.
  //
  // STORAGE RELEASED ('a'):
  //
  // Any storage associated with the invocation's automatic variables is no
  // longer reserved for them.

void demonstrateDanglingPointer() {
    std::cout << "\n=== DANGLING POINTER ===\n";

    Tracer *a{new Tracer};

    delete a;

    // 'a' still exists but its value is now a dangling pointer.
    //
    // The Tracer object's lifetime has already ended, so no live Tracer
    // object exists at the stored address.
    //
    // Dereferencing 'a' attempts to access an object that no longer exists,
    // which is undefined behaviour.
    //
    // std::cout << "  " << a->getId();
    //
    // If AddressSanitizer is enabled, executing the above line would typically
    // be reported as a heap-use-after-free error because it attempts to read
    // data from a heap object whose lifetime has already ended.
}

void demonstrateMemoryLeak() {
    std::cout << "\n=== MEMORY LEAK ===\n";

    Tracer *a{new Tracer};

    // a = new Tracer;

    // 'a' is reassigned to point to a newly allocated Tracer object without
    // deleting the previous Tracer object. Because there are no remaining
    // pointers to the previous Tracer object, it becomes unreachable and
    // therefore cannot be freed. As a result, a memory leak occurs.
    //
    // If LeakSanitizer is enabled, executing the reassignment code would
    // report a memory leak, specifying how many bytes were leaked and in how
    // many allocations.

    delete a;
}

void demonstrateDanglingStackPointer() {
    std::cout << "\n=== DANGLING STACK POINTER ===\n";

    Tracer *ptr{nullptr};

    {
        Tracer a;

        ptr = &a;
    }

    std::cout << ptr;

    // 'ptr' is now a dangling pointer.
    //
    // It still stores the address of 'a', but the lifetime of 'a' ended when
    // the block was exited. No live Tracer object exists at that address.
    //
    // Dereferencing 'ptr' would therefore be undefined behaviour.
    //
    // std::cout << ptr->getId();
    //
    // If AddressSanitizer is enabled, executing the above line would be
    // reported as a stack-use-after-scope error because it attempts to
    // read data from a stack object whose lifetime has already ended.
}

// OBSERVATIONS:
//
// In this run, stack objects (Tracer 0,1,6) have high virtual addresses,
// whereas heap objects (Tracer 2,3,4,5) have low virtual addresses. This
// matches the typical virtual memory layout used by Linux x86-64 processes.

int main() {
    std::cout << "=== STACK VS HEAP LIFETIME INTERACTIONS ===\n";

    demonstrateStackLifetime();
    demonstrateHeapLifetime();
    demonstrateDanglingPointer();
    demonstrateMemoryLeak();
    demonstrateDanglingStackPointer();

    std::cout << "\n=== ALL DEMONSTRATIONS COMPLETE ===\n";
    std::cout << "If AddressSanitizer reports no leaks, all heap objects were "
                 "properly freed.\n";

    return 0;
}
