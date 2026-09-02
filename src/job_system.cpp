#include <jobsys/job_system.hpp>

using namespace jobsys;

JobSystem::JobSystem(int num_workers) {}

JobSystem::~JobSystem(){

}

void JobSystem::submit(Task *task){

}
           
void JobSystem::wait_all(){

}

/*Internal helpers exposed to Worker [for utility purposes]*/
Worker& JobSystem::worker_at(int index){
   
}

void JobSystem::on_task_finished(Task *task){

}