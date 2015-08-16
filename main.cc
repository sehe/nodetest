#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <iostream>
#include <atomic>
#include <future>

#include <iostream>
#include <alsa_control.h>

using std::cout;
using std::endl;

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> MyShmString;
typedef boost::interprocess::allocator<MyShmString, boost::interprocess::managed_shared_memory::segment_manager> StringAllocator;
typedef boost::interprocess::vector<MyShmString, StringAllocator> MyShmStringVector;


class lambda_class {
public:
    void lambda_callback(char *c, int rc) {
      this->sample_count_ += rc;
      this->output_file_.write(c, rc * 2);
    }

    lambda_class(std::string filename) {
      this->filename_ = filename;
      this->sample_count_ = 0;
      this->output_file_.open(this->filename_, std::ios::binary);
      write_header_wav(this->output_file_, 16000, 16, MONO, 10000);
    }

    ~lambda_class() {
      /*
       *this->output_file_.close();
       *this->output_file_.open(this->filename_, std::ios::binary | std::ios::in);
       */
      output_file_.seekp(0ul);
      write_header_wav(this->output_file_, 16000, 16, MONO, this->sample_count_);
    }

private:
    std::string filename_;
    int sample_count_;
    std::ofstream output_file_;
    lambda_class(const lambda_class &a) = delete;
};

class input_class {
public:
    input_class() {
      boost::interprocess::shared_memory_object::remove("MySharedMemory");
      this->shm = new boost::interprocess::managed_shared_memory(boost::interprocess::create_only, "MySharedMemory",
                                                                 1000000);
      CharAllocator charallocator(this->shm->get_segment_manager());
      StringAllocator stringallocator(this->shm->get_segment_manager());
      this->myshmvector = shm->construct<MyShmStringVector>("myshmvector")(stringallocator);
    };

    ~input_class() {
      lambda_class *lc = new lambda_class("listener_vector.wav");
      char *c = (char *) malloc(2048);
      for (MyShmStringVector::iterator it = this->myshmvector->begin(); it != this->myshmvector->end(); it++) {
        strcpy(c, it->c_str());
        lc->lambda_callback(c, 2048);
      }
      delete lc;
      boost::interprocess::shared_memory_object::remove("MySharedMemory");
      this->shm->destroy_ptr(this->myshmvector);
    }

    void to_node(char *c, int rc) {
      CharAllocator charallocator(this->shm->get_segment_manager());
      StringAllocator stringallocator(this->shm->get_segment_manager());
      MyShmString mystring(charallocator);
      mystring = c;
      this->myshmvector->insert(this->myshmvector->begin(), mystring);
    }

private:
    boost::interprocess::managed_shared_memory *shm;
    MyShmStringVector *myshmvector;
};

void listener() {
    lambda_class ctc("writer.wav");

    char *c = (char *) malloc(2048);

    boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, "MySharedMemory");
    MyShmStringVector *myvector = segment.find<MyShmStringVector>("myshmvector").first;


    for (MyShmStringVector::iterator it = myvector->begin(); it != myvector->end(); it++) {
        //strcpy(c, std::string(it->begin(), it->end()).c_str()); //HUHUH!?!
        strcpy(c, it->c_str());
        ctc.lambda_callback(c, 2048);
    }
}

int main(int argc, char **argv) {
    alsa_control ac(16000, 2048, 16, MONO);

    if (argc == 1) {
        input_class ic;
        ac.listen_with_callback(std::bind(&input_class::to_node, &ic, std::placeholders::_1, std::placeholders::_2), "listener");
        sleep(5);
        ac.stop();
        cout << "Go" << endl;
        sleep(10);
    } else {
        //    std::atomic<bool> done(false);
        auto th = std::async(std::launch::async, listener);
        //    done.store(true, std::memory_order_relaxed);
        th.get();
    }
    return 0;
}
