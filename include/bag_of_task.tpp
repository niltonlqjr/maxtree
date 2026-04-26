

template <class Task>
bag_of_tasks<Task>::bag_of_tasks(bool start_running){
    this->tasks = new std::deque<Task>();
    
    this->running = start_running;
    this->waiting = 0;
}
template <class Task>
bag_of_tasks<Task>::~bag_of_tasks(){
    delete this->tasks;
}
template <class Task>
void bag_of_tasks<Task>::start(){
    std::unique_lock<std::mutex> l(this->lock);
    this->running = true;
}

template <class Task>
void bag_of_tasks<Task>::insert_task(Task t){
    // std::cout << "insert task\n";
    std::unique_lock<std::mutex> l(this->lock);
    this->tasks->push_back(t);
    
    this->wakeup_workers();
    // this->has_task.notify_all();
}

template <class Task>
int bag_of_tasks<Task>::position_of(int priority){
    /*elaborar uma formula para obter a posição dada uma prioridade
    pensar numa forma de limitar as prioridades;*/
    return priority;
}

template <class Task>
bool bag_of_tasks<Task>::is_running(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->running;
}

template <class Task>
bool bag_of_tasks<Task>::get_task(Task &ret){
    // std::cout << "get task - tasks->size():" << this->tasks->size() << "\n";
    std::unique_lock<std::mutex> l(this->lock);

    while(this->tasks->size() <= 0 && this->running){
        // this->waiting++;
        // std::cout << "wait task\n";
        this->has_task.wait(l);
        // std::cout << "wait task\n";

    }
    if(this->tasks->size() > 0){
        ret = this->tasks->front();
        // std::cout << ret << "\n";
        this->tasks->pop_front();
        
        this->wakeup_workers();
        return true;
    }else{
        return false;
    }
    
}

template <class Task>
bool bag_of_tasks<Task>::get_task_by_position(Task &ret, size_t position){
    // std::cout << "get task by position - tasks->size():" << this->tasks->size() << " tasks->size:" << this->tasks->size() << "\n";
    std::unique_lock<std::mutex> l(this->lock);
    while(this->tasks->size() <= 0 && this->running){
        this->has_task.wait(l);
    }
    if(this->tasks->size() > 0 && position < this->tasks->size()){
        ret = this->tasks->at(position);
        this->tasks->erase(this->tasks->begin() + position);
        
        this->no_task.notify_all();
        return true;
    }
    return false;
    
}

template <class Task>
template <class T> 
bool bag_of_tasks<Task>::get_task_by_function(Task &ret, T value, T function(Task)){
    // std::cout << "get task by function - tasks->size():" << this->tasks->size() << " tasks->size:" << this->tasks->size() << "\n";
    std::unique_lock<std::mutex> l(this->lock);
    Task t;

    // std::deque<Task>::iterator it;
    for(auto it=this->tasks->begin(); it != this->tasks->end(); it++ ){
        t = *it; 
        if(value == function(t)){
            ret = t;
            
            this->tasks->erase(it);
            this->no_task.notify_all();
            return true;
        }
    }
    return false;

    // catch(std::runtime_error &e){
    //     return false;
    // }catch(std::out_of_range &e){
    //     return false;
    // }
}

template <class Task>
Task bag_of_tasks<Task>::at(int pos){
    std::unique_lock<std::mutex> l(this->lock);
    return this->tasks->at(pos);
}

template <class Task>
void bag_of_tasks<Task>::wait_empty(){
    std::unique_lock<std::mutex> l(this->lock);
    while(this->tasks->size() > 0){
        this->no_task.wait(l);
    }
}

template <class Task>
void bag_of_tasks<Task>::wakeup_workers(){
    this->waiting=0;
    this->has_task.notify_all();
}

template <class Task>
void bag_of_tasks<Task>::notify_end(){
    std::unique_lock<std::mutex> l(this->lock);
    this->running = false;
    this->wakeup_workers();
}

template <class Task>
int bag_of_tasks<Task>::num_waiting(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->waiting;
}

template <class Task>
int bag_of_tasks<Task>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->tasks->size();
}

template <class Task>
void bag_of_tasks<Task>::print(){
    std::unique_lock<std::mutex> l(this->lock);
    this->tasks->print();
}

template <class Task>
bool bag_of_tasks<Task>::empty(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->tasks->size() == 0;
}


template <class Task>
template <class T>
uint64_t bag_of_tasks<Task>::search_by_function(T value, T function(Task)){
    std::cout << "search_by_function\n";
    std::unique_lock<std::mutex> l(this->lock);
    
    uint64_t i;
    for(i = 0; i < this->tasks->size(); i++){
        if(value == function(this->tasks->at(i))){
            return i;
        }
    }
    throw std::runtime_error("error on bag_of_task::search_by_field: Task not found on bag\n");
}

/*==============================================================================================================
  =======================================     Priority bag of tasks     ========================================
  ==============================================================================================================*/


template <class Task>
prio_bag_of_tasks<Task>::prio_bag_of_tasks(bool start_running){
    this->tasks = new min_heap<Task>();
    this->running = start_running;
    this->waiting = 0;
}
template <class Task>
prio_bag_of_tasks<Task>::~prio_bag_of_tasks(){
    delete this->tasks;
}

template <class Task>
void prio_bag_of_tasks<Task>::insert_task(Task t){
    // std::cout << "prio insert task\n";
    std::unique_lock<std::mutex> l(this->lock);
    this->tasks->insert(t);
    this->wakeup_workers();
    // this->has_task.notify_all();
}

template <class Task>
bool prio_bag_of_tasks<Task>::get_task(Task &ret, int priority){
    // std::cout << "prio get task tasks->size():" << this->tasks->size() << " tasks->size:" << this->tasks->size() << "\n";
    std::unique_lock<std::mutex> l(this->lock);
    int pos;
    if(priority == -1){
        pos = 0;
    }else{
        pos = this->position_of(priority);
    }
    while(this->tasks->size() <= 0 && this->running){
        this->waiting++;
        this->has_task.wait(l);
    }
    // if(!this->running){
    //     return false;
    if(this->tasks->size() > 0){
        ret = this->tasks->at(pos);
        this->tasks->remove_at(pos);
        
        return true;
    }else{
        return false;
    }
    
}

/*==============================================================================================================
  ========================================     Ordered bag of tasks     ========================================
  ==============================================================================================================*/

template <class Task, bool CompareLesser(Task, Task)>
ordered_bag_of_tasks<Task, CompareLesser>::ordered_bag_of_tasks(bool start_running){
    this->tasks = new std::deque<Task>();
    
    this->running = start_running;
    this->waiting = 0;
}
template <class Task, bool CompareLesser(Task, Task)>
ordered_bag_of_tasks<Task, CompareLesser>::~ordered_bag_of_tasks(){
    delete this->tasks;
}

template <class Task, bool CompareLesser(Task, Task)>
void ordered_bag_of_tasks<Task, CompareLesser>::insert_task(Task t){
    // std::cout << "ordered insert task:" << this->tasks->size() << "\n";
    std::unique_lock<std::mutex> l(this->lock);
    this->tasks->push_back(t);
    
    size_t i=this->tasks->size()-1;
    while(i > 0 && CompareLesser(this->tasks->at(i-1) , t)){
        this->tasks->at(i) = this->tasks->at(i-1);
        i--;
    }
    this->wakeup_workers();
}
