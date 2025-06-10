# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\p221t801\Downloads\PHC_2\ws\base_2025_06_10_03_46_pm\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\p221t801\Downloads\PHC_2\ws\base_2025_06_10_03_46_pm\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {base_2025_06_10_03_46_pm}\
-hw {C:\Users\p221t801\Downloads\PHC_2\base_2025_06_10_03_46_pm.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -out {C:/Users/p221t801/Downloads/PHC_2/ws}

platform write
platform generate -domains 
platform active {base_2025_06_10_03_46_pm}
platform generate
