# Digital-christmas-star

Code for this [video](https://www.youtube.com/watch?v=jLeo_D0pDiA)

## Paramters that need to be changed before upload
* DATA_PIN -> The IO pin to which the LED's are connnected
* NUM_FINS -> The number of fins (for a regular star 4-5)
* NUM_LEDS_BETWEEN_TIPS -> The number of LED's between the tips (not including tips)
* animationTime -> The time in milliseconds how long each animation is shown

## Adding new animations
* Add a new animation function
* Add a new case to the switch case
* Call the function in the created case
* Increase maxMode by 1 (set it to the same number as the last number of the switch case)