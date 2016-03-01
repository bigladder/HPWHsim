library(EcotopePackage)
library(foreign)

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
wd <- getwd()


dir.create("models")
copyLabData <- function(make) {
  print(paste("Copying sim parameters for make", make))
  dir.create(paste(wd, "models", make, sep = "/"))
  
  cdx("hpwhlab")
  dfiles <- dir("../data/prepped/", pattern = paste("_", make, "_", sep = ""))
  # What to do about reduced airflow tests???
  
  # For each test, write out the real results, and also a folder for testTool inputs
  sapply(dfiles, function(dfile) {
    print(paste("Copying sim parameters for file", dfile))
    if(length(grep("AF", dfile))) {  # Get rid of the reduced airflow tests???
      return(NULL)
    }
    cdx("hpwhlab")
    dset <- read.dta(paste("../data/prepped", dfile, sep = "/"))
    dset$time <- stata_time_to_R_time(dset$time)
    dset <- dset[dset$minutes >= 0, ] # Sometimes the dataset does not start at minute zero!
    
    # Collect the relevant info to run the test tool
    inlet <- mean(dset$T_In_water, na.rm = TRUE)
    setpoint <- quantile(dset$T_Out_water, .95, na.rm = TRUE)
    Ta <- mean(dset$T_Plenum_In, na.rm = TRUE)
    
    test_minutes <- sum(!is.na(dset$flow_out_gal))
    flowDset <- dset[ , c("minutes", "flow_out_gal")]
    flowDset <- flowDset[!is.na(flowDset[, 2]), ]
    flowDset <- flowDset[flowDset[, 2] > 0, ]
    if(sum(flowDset[, 2]) == 0) {
      setpoint <- dset$T_Tank_ave[1]
    }
    
    setwd(wd)
    # Write out schedule files for the test tool
    # Write out the draw schedule
    test <- gsub("^(.+)_(.+)_(.+).dta$", "\\1_\\3", dfile)
    dir2 <- paste("models", make, test, sep = "/")
    dir.create(dir2)
    drawScheduleFile <- paste(dir2, "drawschedule.csv", sep = "/")
    writeLines(text = "default 0", con = drawScheduleFile)
    write.table(flowDset, file = drawScheduleFile, row.names = FALSE,
                append = TRUE, sep = ",", quote = FALSE)
    
    # Write out the ambient schedule. Do I want to add
    ambientScheduleFile <- paste(dir2, "ambientTschedule.csv", sep = "/")
    writeLines(text = paste("default", Ta), con = ambientScheduleFile)
    
    # Write out the evaporator schedule
    evaporatorScheduleFile <- paste(dir2, "evaporatorTschedule.csv", sep = "/")
    writeLines(text = paste("default", Ta), con = evaporatorScheduleFile)
    
    # Write out the inlet schedule
    inletScheduleFile <- paste(dir2, "inletTschedule.csv", sep = "/")
    writeLines(text = paste("default", inlet), con = inletScheduleFile)
    
    # Write out the DR schedule
    DRScheduleFile <- paste(dir2, "DRschedule.csv", sep = "/")
    writeLines(text = "default 1\nminutes,OnOff", con = DRScheduleFile)
    
    # What was the setpoint???
    testInfoFile <- paste(dir2, "testInfo.txt", sep = "/")
    writeLines(text = paste0("length_of_test ", test_minutes,
                             "\nsetpoint ", setpoint),
               con = testInfoFile)
    
    dset <- dset[, names(dset) %in% c("time", "T_Tank_02", "T_Tank_04", "T_Tank_06", "T_Tank_08",
                                      "T_Tank_10", "T_Tank_12", "Power_W", "Power_res_kW",
                                      "Power_comp_kW", "flow_out_gpm", "T_In_water",
                                      "T_Out_water", "minutes", "Power_kW",
                                      "T_Tank_ave", "T_Tank_COP", "flow_out_gal", "dEadd5min", "T_Plenum_In",
                                      "COP5min")]
    names(dset)[names(dset) == "flow_out_gal"] <- "flow"
    names(dset)[names(dset) == "T_Tank_02"] <- "tcouples1"
    names(dset)[names(dset) == "T_Tank_04"] <- "tcouples2"
    names(dset)[names(dset) == "T_Tank_06"] <- "tcouples3"
    names(dset)[names(dset) == "T_Tank_08"] <- "tcouples4"
    names(dset)[names(dset) == "T_Tank_10"] <- "tcouples5"
    names(dset)[names(dset) == "T_Tank_12"] <- "tcouples6"
    dset$inputPower <- dset$Power_W
    if(is.null(dset$dEadd5min)) {
      # Weird problem with one of the Stiebel tests...
      return(NULL)
    }
    dset$outputPower <- dset$dEadd5min / 5 / 60 * 1000
    
    tempVars <- grep("tcouples", names(dset))
    dset[, tempVars] <- dset[, tempVars] * 1.8 + 32
    dset$aveTankTemp <- apply(dset[, tempVars], 1, mean)
    
    # Write out the full version
    write.csv(file = paste("models/", make, "/", test, "_Full.csv", sep = ""),
              dset, row.names = FALSE)
    
    # Write out a version that is easy to read into C++
#     dset <- dset[, c("inputPower", "aveTankTemp")]
#     
#     write.table(file = paste("models/", make, "/", test, ".csv", sep = ""),
#                 dset, row.names = FALSE, sep = " ")
  })
  
  
}


makes <- c("AOSmith50", "AOSmith60", "AOSmith80",
           "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
           "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
           "SandenGAU", "Stiebel220e")
lapply(makes, copyLabData)
