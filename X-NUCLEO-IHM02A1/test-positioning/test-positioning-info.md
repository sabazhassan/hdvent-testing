The motor curve is parametrized by the following variables
- Inspiratory phase (compressing ambu bag)
  - `acc_in` Acceleration in
  - `speed_in` Constant speed in
  - `dec_in` Deceleration in
  - `time_hold_plateau` time to wait after compressing before releasing
  - `steps_interval` 
 
- Expiratory phase (releasing ambu bag)
  - `acc_ex` Acceleration out
  - `speed_ex` Constant speed out
  - `dec_ex` Deceleration out
  - `steps_interval`
  - time after release (to meet respiration rate, breaths per minute)
  
All variables will probably be constants (hard coded in the script), except 
`speed_in` and `steps_interval`, which are calculated from the user-set variables
 - `respiratory_rate` Breaths per minute
 - `path_ratio` which portion of total path should be driven
 - `ie_ratio` ratio between inhalation and exhalation time e.g. 1:2(=0.5)

