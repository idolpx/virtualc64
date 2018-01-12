//
//  SepiaFilter.h
//  V64
//
//  Created by Dirk Hoffmann on 18.10.15.
//
//

#import "ComputeKernel.h"

@interface SepiaFilter : ComputeKernel
{
}

+ (instancetype) forDevice:(id <MTLDevice>)dev fromLibrary:(id <MTLLibrary>)lib;

@end
