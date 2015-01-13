<?

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

require_once('DataFunctions.php');
?>

#ifndef _EXEC_ENGINE_TASKS_H_
#define _EXEC_ENGINE_TASKS_H_

#include <cinttypes>
#include <string>

#include "TwoWayList.h"

// Base class for Execution Engine Tasks
<?
grokit\create_base_data_type( "Task", "DataC", [], [], true, true);
?>

typedef TwoWayList<Task> TaskList;

// Delete Relation
<?
grokit\create_data_type("DeleteRelationTask", "Task", [ 'relation' => 'std::string' ], [], true);
?>

<?
grokit\generate_deserializer( 'Task' );
?>

#endif // _EXEC_ENGINE_TASKS_H_
