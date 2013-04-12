CBVision is a video filtering tool designed to reduce the gross volume of video that must be reviewed by human annotators. It has been developed to facilitate fish tracking in hydropower dam fish ladders. CBVision uses background subtraction to remove video frames in which nothing of interest is occurring. On frames in which there is something possibly of interest, CBVision tracks likely targets. If enough frames show an object of consistent size and shape moving in the frame, CBVision deems the series of objects an "event" and retains it. Brief and non-contiguous objects are thrown out. 
At present, CBVision relies on human annotators to classify what sort of fish has been seen. 
Ongoing research and development objectives for CBVision are:
* Event classification algorithm
* Packaging and porting
* Graphical user interface
CBVision is primarily the work of UC Davis researcher Cristi Negrea. 
We are releasing this under GNU GPL 2.0. 

