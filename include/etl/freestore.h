/*
 * freestore.h
 *
 * Created: 05/03/2014 23:02:55
 *  Author: Ambroise Leclerc
 */ 


#ifndef ETL_FREESTORE_H_
#define ETL_FREESTORE_H_

namespace etl {
class FreeStore {
 public:
  /// Allocates size bytes of uninitialized storage.
  /// @param[in] size number of bytes to allocate
  /// @return Pointer to the beginning of allocated memory. The pointer must be
  /// deallocated with FreeStore::Deallocate.
  static void* Allocate(std::size_t size) noexcept {
    auto chunk_parser = reinterpret_cast<Chunk*>(free_chunk_);
      
    // 0 Setup the start of allocatable area (data_) to the first unallocated block
    while (chunk_parser->IsAllocated()) {
      chunk_parser = chunk_parser->NextChunk();
      free_chunk_ = reinterpret_cast<uint8_t*>(chunk_parser);
    }
    auto second_choice = chunk_parser;
    uint16_t second_choice_size = 0b1111111111111u;
      
    // 1 Find a free chunk with the same size as the requested allocation size.
    while (!chunk_parser->IsLastChunk()) {
      if (!chunk_parser->IsAllocated()) {
        if (chunk_parser->ChunkSize() == size) {
          return chunk_parser->AllocateData(size);
        }
        if (chunk_parser->ChunkSize() > (size+1)) {
          if (chunk_parser->ChunkSize() < second_choice_size) {
            second_choice = chunk_parser;
            second_choice_size = chunk_parser->ChunkSize();
          }
        }
      }
      chunk_parser = chunk_parser->NextChunk();
    }

    // 2 Free chunk with exact same size has not been found : We allocate the second choice
    if (second_choice->ChunkSize() < (size+2)) {
      second_choice = chunk_parser;
    }
    return second_choice->AllocateData(size);
  }

  /// Deallocates the memory space pointed to by ptr, which must have been
  /// returned by a previous call to FreeStore::Allocate(). Otherwise undefined
  /// behaviors occurs.
  /// @param[in] pointer to the memory to deallocate
  /// @return
  static void Deallocate(void* ptr) noexcept {
    auto deallocatable = reinterpret_cast<uint8_t*>(ptr);
    deallocatable -= CompileTimeOffsetOf(&Chunk::data);           // Point to chunk beginning
    if (deallocatable < free_chunk_) {                            // if chunk to deallocate is before the current first free chunk
      free_chunk_ = reinterpret_cast<uint8_t*>(deallocatable);    // then we define it's address as the new first free chunk
    }
    reinterpret_cast<Chunk*>(deallocatable)->Deallocate();
  }
    
  /// Returns how many bytes have been allocated.
  /// @return allocated bytes
  static std::size_t GetMemorySize() noexcept {
    return GetChunksSize(false);
  }

  /// Computes storage space overhead caused by memory fragmentation. A 0%
  /// fragmentation level means that memory is allocated optimally. A 100%
  /// level indicates that fragmentation doubles the memory space needs. More
  /// than 100% is a final warning for an inevitable failure.
  /// @return Fragmentation level in percents
  static uint8_t GetMemoryFragmentation() noexcept {
    uint16_t holessize = GetChunksSize(true);
    uint16_t memsize = GetMemorySize();
    if (0 == memsize) return 0;
    return 100-((100 * memsize) / (memsize + holessize));
  }

  /// Computes available RAM (i.e. total microcontroller on-board SRAM
  /// minus global variables, stack and free store consumption).
  /// @return Available RAM in bytes
  static std::size_t GetFreeMemory() noexcept {
    uint16_t stack_limit = 0;
    auto mem_spread = reinterpret_cast<std::size_t>(&stack_limit);
    Chunk* chunk_parser = reinterpret_cast<Chunk*>(free_chunk_);
    while (!chunk_parser->IsLastChunk()) {
      chunk_parser = chunk_parser->NextChunk();
    }
    mem_spread -= reinterpret_cast<std::size_t>(chunk_parser);
    return mem_spread;
  }


 private:
  struct Chunk {
    enum Bits : uint16_t { AllocatedChunk = 1u<<15, LastChunk = 1u<<14 };

    uint16_t descriptor;    // bit0-12:size  bit15:allocated bit14:Last
    uint8_t data;

    void SetChunkSize(uint16_t size) { descriptor = (descriptor & 0b1100000000000000) | size; }
    uint16_t constexpr ChunkSize() const  { return descriptor & 0b1111111111111; }

    bool constexpr IsAllocated() const    { return (descriptor & AllocatedChunk) == AllocatedChunk; }
    void SetAllocated()                   { descriptor |= AllocatedChunk; }
    void ResetAllocated()                 { descriptor &= ~AllocatedChunk; }

    bool constexpr IsLastChunk() const    { return (descriptor & LastChunk) == LastChunk; }
    void SetLastChunk()                   { descriptor |= LastChunk; }
    void ResetLastChunk()                 { descriptor &= ~LastChunk; }
      
    Chunk* NextChunk() { return reinterpret_cast<Chunk*>((&data) + ChunkSize()); }

    void* AllocateData(uint16_t size) {
      uint16_t next_size = ChunkSize() - size;
      SetChunkSize(size);
      descriptor |= AllocatedChunk;
      if (next_size != 0) {
        NextChunk()->descriptor = next_size;   // Set next chunk size and clear descriptor's high bits
        if (IsLastChunk()) {
          NextChunk()->SetLastChunk();
          ResetLastChunk();
        }
      }
      return &data;
    }

    void Deallocate() {
      // Deallocation of this chunk :
      //  - if next chunk is unallocated then we have to merge both chunks
      //  - if not, we simply mark this chunk has unallocated
      if (!NextChunk()->IsAllocated()) {
        SetChunkSize(ChunkSize() + NextChunk()->ChunkSize());
      }
      ResetAllocated();
    }
  };

  static std::size_t GetChunksSize(bool allocated) {
    Chunk* chunk_parser = reinterpret_cast<Chunk*>(__malloc_heap_start);
    uint16_t size = 0;
      
    while (allocated == chunk_parser->IsLastChunk()) {
      if (chunk_parser->IsAllocated()) {
        size += chunk_parser->ChunkSize();
      }
      chunk_parser = chunk_parser->NextChunk();
    }
    return size;
  }

 private:
  friend class HardwareInitializer;
  static bool Initialize() noexcept {
    reinterpret_cast<Chunk*>(free_chunk_)->descriptor = ~(0);
    reinterpret_cast<Chunk*>(free_chunk_)->SetLastChunk();
    reinterpret_cast<Chunk*>(free_chunk_)->ResetAllocated();
    return true;
  }
  static uint8_t* free_chunk_;
};

uint8_t* FreeStore::free_chunk_ = reinterpret_cast<uint8_t*>(__malloc_heap_start);

} // namespace etl

#endif // ETL_FREESTORE_H_