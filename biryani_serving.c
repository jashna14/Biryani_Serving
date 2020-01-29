#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int number_of_students , number_of_chefs, number_of_tables, total_number_of_hungry;

typedef struct Student{
    int idx;
    pthread_t student_thread_id;
    int table_id;
    int status;
}Student;

typedef struct Chef{
    int idx;
    pthread_t chef_thread_id;
    pthread_mutex_t mutex;
    int number_of_vessels;
    int capacity_of_vessels;
    int finished_number_of_vessels;
    pthread_cond_t cond;
    int status;
}Chef;

typedef struct Table{
    int idx;
    pthread_t table_thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_cond_t cond1;
    int total_number_of_slots;
    int current_number_of_slots;
    int *students_eaten;
    int status;
    int pointer;
}Table;

Student **student;
Chef **chef;
Table **table;
int  biryani_ready(void *args);
void * biryani_preparing(void *args);
void *student_in_slot(void *args);
void *wait_for_slot(void *args);
void * ready_to_serve(void *args);
void * waiting_to_serve(void *args);

int pop(int *arr, int pointer)
{
    if(pointer > -1)
    {    
        return(arr[pointer]);
    }    
}

int push(int *arr, int pointer, int x)
{   
    if(pointer > -2)
    {
        arr[pointer] = x;
    }    
}

int min(int a , int b)
{
    if(a<b)
    {
        return a;
    }
    else
    {
        return b;
    }
    
}
int biryani_ready(void *args)
{
    if(!total_number_of_hungry)
    {
        return 0;
    }   

    Chef *chef = (Chef*)args;
    pthread_mutex_t * mutex_point = &(chef->mutex);
    pthread_mutex_lock(mutex_point);
    if(total_number_of_hungry && chef->number_of_vessels != chef->finished_number_of_vessels)
        pthread_cond_wait(&(chef->cond),mutex_point);
    pthread_mutex_unlock(mutex_point);

    if(chef->number_of_vessels != chef->finished_number_of_vessels)
    {
        printf("All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now.\n",chef->idx+1);
        fflush(stdout);
    }

    return 0;

}

void * biryani_preparing(void *args)
{
    if(!total_number_of_hungry)
    {
        return NULL;
    }   

    Chef *chef = (Chef*)args;
    while(!chef->status)
    {
        if(!total_number_of_hungry)
        {
            return NULL;
        } 
        int time_for_preparing = (2 + (rand()%4));
        chef->number_of_vessels = 1 + rand()%10;
        chef->finished_number_of_vessels = 0;
        chef->capacity_of_vessels = (25 + (rand()%26));
        printf("Robot Chef %d is preparing %d vessels of Biryani\n",chef->idx+1,chef->number_of_vessels);
        fflush(stdout);
        sleep(time_for_preparing);
        if(total_number_of_hungry)
        {
            printf("Robot Chef %d has prepared %d vessels of Biryani\n.Waiting for all the vessels to be emptied to resume cooking\n",chef->idx+1,chef->number_of_vessels);
            fflush(stdout);
        }
        chef->status = 1;
        chef->status = biryani_ready(chef);
    }    
}

void *student_in_slot(void *args)
{   
    Student *student = (Student*)args;
    printf("student %d assigned a slot on the serving table %d and waiting to be served\n",student->idx+1,student->table_id+1);
    fflush(stdout);

}

void *wait_for_slot(void *args)
{   
    sleep(rand()%10);
    Student *student = (Student*)args;
    printf("Student %d has arrived\n",student->idx+1);
    fflush(stdout);
    sleep(0.5);
    printf("Student %d is waiting to be allocated a slot on the serving table\n",student->idx+1);
    fflush(stdout);
    while(student->status == 0)
    {
        for(int i=0;i<number_of_tables;i++)
        {
            pthread_mutex_t * mutex_point = &(table[i]->mutex);
            pthread_mutex_lock(mutex_point);
            if(table[i]->status == 1)
            {
                student->status = 1;
                student->table_id = i;
                table[i]->current_number_of_slots--;
                table[i]->pointer ++;
                push(table[i]->students_eaten, table[i]->pointer, student->idx);
                total_number_of_hungry-- ;
                if(table[i]->current_number_of_slots == 0 || total_number_of_hungry == 0)
                {   
                    pthread_cond_broadcast(&(table[i]->cond));
                	table[i]->status =0;
                }
                pthread_mutex_unlock(mutex_point);
                student_in_slot(student);

                break;
            }
            else
            {
                pthread_mutex_unlock(mutex_point);
            }
            
        }
    }
}
void * ready_to_serve(void *args)
{   
    if(!total_number_of_hungry)
    {
        return NULL;
    }

    Table *table = (Table*)args;
    pthread_mutex_t * mutex_point = &(table->mutex);
    pthread_mutex_lock(mutex_point);
    if(table->current_number_of_slots && total_number_of_hungry)
        pthread_cond_wait(&(table->cond),mutex_point);
    pthread_mutex_unlock(mutex_point);
    sleep(1);

    if(table->pointer > -1)
    {
        printf("Serving table %d entering Serving Phase\n",table->idx+1);    
        fflush(stdout);
        while(table->pointer > -1)
        {	
            printf("student %d on serving table %d has been served\n",pop(table->students_eaten,table->pointer) + 1,table->idx+1);
            fflush(stdout);
            table->pointer --;
        }
    }
    table->status = 0;    
}

void * waiting_to_serve(void *args)
{
    if(!total_number_of_hungry)
    {
        return NULL;
    }

    Table *table = (Table*)args;
    if(table->total_number_of_slots == 0)
    {
        printf("Serving Container of Table %d is empty, waiting for refill\n",table->idx+1);
        fflush(stdout);
    }
    while(1)
    {   
        if(!total_number_of_hungry)
        {
            return NULL;
        }

        if(table->total_number_of_slots == 0)
        {
            for(int i=0;i<number_of_chefs;i++)
            {
                pthread_mutex_t * mutex_point = &(chef[i]->mutex);
                pthread_mutex_lock(mutex_point);
                if(chef[i]->status == 1)
                {
                    table->total_number_of_slots = chef[i]->capacity_of_vessels;
                    table->current_number_of_slots = min((1+rand()%10),table->total_number_of_slots);
                    // table->current_number_of_slots = 1 + (rand()%10)%(table->total_number_of_slots;
                    table->total_number_of_slots -= table->current_number_of_slots;
                    table->pointer = -1; 
                    chef[i]->finished_number_of_vessels ++;
                    if(chef[i]->number_of_vessels == chef[i]->finished_number_of_vessels)
                    {
                        pthread_cond_broadcast(&(chef[i]->cond));
                    	chef[i]->status =0;
                    }
                    if(total_number_of_hungry)
                    {   
                        printf("Robot Chef %d is refilling Serving Container of Serving Table %d\n",chef[i]->idx+1,table->idx+1); 
                        fflush(stdout);
                        sleep(1);
                        printf("Serving Container of Table %d is refilled by Robot Chef %d; Table %d resuming serving now\n",table->idx+1,chef[i]->idx+1,table->idx+1);
                        fflush(stdout);
                        printf("Serving Table %d is ready to serve with %d slots\n",table->idx+1,table->current_number_of_slots);
                        fflush(stdout);
                    }
                    table->status = 1;
                    pthread_mutex_unlock(mutex_point);
                    ready_to_serve(table);
                    sleep(2);
                    break;
                }
                else
                {
                    pthread_mutex_unlock(mutex_point);
                }
                
            }
        }
        else
        {
            table->current_number_of_slots = min((1+rand()%10),table->total_number_of_slots);
            table->total_number_of_slots -= table->current_number_of_slots;
            table->pointer = -1;
            if(total_number_of_hungry)
            {        
                printf("Serving Table %d is ready to serve with %d slots\n",table->idx+1,table->current_number_of_slots);
                fflush(stdout);
            }
            table->status = 1;
            sleep(2);
            ready_to_serve(table);
        }
            
    }    

}



int main()
{
    int i,j;

    printf("Please enter the number of Robot Chefs\n");
    scanf("%d",&number_of_chefs);

    printf("Please enter the number of Serving Tables\n");
    scanf("%d",&number_of_tables);

    printf("Please enter the number of Students\n");
    scanf("%d",&number_of_students);

    if(!number_of_students || !number_of_chefs || !number_of_tables)
    {
        printf("All the inputs should neccessarily be greater than zero\n");
        return 0;
    }

    total_number_of_hungry  = number_of_students;

    chef = (Chef**)malloc((sizeof(Chef*))*number_of_chefs);
    
    for(i=0;i<number_of_chefs;i++)
    {
        chef[i] = (Chef*)malloc(sizeof(Chef));
        chef[i]->idx = i;
        chef[i]->status = 0;
        pthread_mutex_init(&(chef[i]->mutex), NULL);
        pthread_cond_init(&(chef[i]->cond), NULL);
    }

    table = (Table**)malloc((sizeof(Table*))*number_of_tables);
    
    for(i=0;i<number_of_tables;i++)
    {
        table[i] = (Table*)malloc(sizeof(Table));
        table[i]->idx = i;
        table[i]->status = 0;
        table[i]->total_number_of_slots = 0;
        pthread_cond_init(&(table[i]->cond), NULL);
        pthread_cond_init(&(table[i]->cond1), NULL);
        pthread_mutex_init(&(table[i]->mutex), NULL);
        table[i]->students_eaten = (int*)malloc(10*sizeof(int));
        table[i]->pointer = -1;
    }

    student = (Student**)malloc((sizeof(Student*))*number_of_students);
    
    for(i=0;i<number_of_students;i++)
    {
        student[i] = (Student*)malloc(sizeof(Student));
        student[i]->idx = i;
        student[i]->status = 0;
        student[i]->table_id = -1;
    }

    for(i=0;i<number_of_chefs;i++)
    {   
        pthread_create(&(chef[i]->chef_thread_id),NULL,biryani_preparing,chef[i]);
    }

    for(i=0;i<number_of_tables;i++)
    {
        pthread_create(&(table[i]->table_thread_id),NULL,waiting_to_serve,table[i]);
    }

    for(i=0;i<number_of_students;i++)
    {
        pthread_create(&(student[i]->student_thread_id),NULL,wait_for_slot,student[i]);
    }

    for(i=0;i<number_of_students;i++)
    {
        pthread_join(student[i]->student_thread_id, 0);
    }

    for(i=0;i<number_of_tables;i++)
    {
    	// if(table[i]->pointer > -1)
    	// {
    	// 	pthread_cond_broadcast(&table[i]->cond);
    	// }
        if(table[i]->pointer > -1)
        {
            printf("Serving table %d entering Serving Phase\n",table[i]->idx+1); 
            fflush(stdout);   
        	while(table[i]->pointer > -1)
        	{   
            	printf("Student %d on serving table %d has been served\n",pop(table[i]->students_eaten,table[i]->pointer) + 1,table[i]->idx+1);
            	fflush(stdout);
            	table[i]->pointer --;
        	}
        }    
    }

    printf("Simulation Over.\n");

    

}