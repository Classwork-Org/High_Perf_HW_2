package main

import (  
    "fmt"
)
const threadCount int = 1000;
const arraySize int = 1000000;
const array1Val int = 4;
const array2Val int = 3;

func vmult(array1 []int, array2 []int, done chan bool){
	for i := 0; i < len(array1); i++ {
		array1[i] *= array2[i];
	}			
	done <- true;
}

func pvmult(array1 []int, array2 []int){	

	var chans [threadCount]chan bool
	for i := range chans {
	   chans[i] = make(chan bool)
	}

	sliceSize := len(array1)/threadCount;

	threadIdx := 0;
	for low := 0; low < len(array1); low+=sliceSize {
		high := low+sliceSize;		
		go vmult(array1[low:high], array2[low:high], chans[threadIdx]);
		threadIdx++;
	}

	for i := range chans {
		<- chans[i];
	}

}

func vadds(array []int, val int) {
	for i := 0; i < len(array); i++ {
		array[i] += val;
	}	
}

func main() {  

	a := make([]int,arraySize,arraySize);
	b := make([]int,arraySize,arraySize);

	vadds(a, 6);

	vadds(b, 4);

	pvmult(a,b);

	fmt.Printf("DONE!\n");

	for i := 0; i < len(a); i++ {
		if(a[i] != 6*4){
			fmt.Printf("ERROR!\n");
		}
	}

}