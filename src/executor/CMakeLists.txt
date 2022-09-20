target_sources(Swiftray PRIVATE
  executor.cpp
  job_executor.cpp
  machine_setup_executor.cpp
  rt_status_update_executor.cpp
  console_executor.cpp
)

add_subdirectory(machine_job)
add_subdirectory(operation_cmd)
