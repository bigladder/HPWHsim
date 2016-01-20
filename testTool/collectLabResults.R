# Collect the lab testing results
library(foreign)
library(EcotopePackage)

cdx("hpwhlab")
prepped <- dir("../data/prepped/")
allTests <- data.frame("fullName" = prepped,
                       "modelName" = gsub("^(.+)_(.+)_(.+)\\.dta$", "\\2", prepped),
                       "testName" = gsub("^(.+)_(.+)_(.+)\\.dta$", "\\1_\\3", prepped),
                       stringsAsFactors = FALSE)
models <- sort(unique(allTests$modelName))
tests <- sort(unique(allTests$testName))

testsToUse <- c("DOE_24hr50", "DOE_24hr67", "DP_SHW50")
modelsToUse <- data.frame("labName" = c("ATI66rev2", "GE502014", "GE502014STDMode", "GEred",
                                        "SandenGES", "AOSmith60"),
                          "simName" = c("ATI66", "GE502014", "GE502014STDMode", "GEred",
                                        "Sanden80", "Voltex60"),
                          stringsAsFactors = FALSE)

allLabResults <- do.call('rbind', lapply(testsToUse, function(test) {
  do.call('rbind', lapply(modelsToUse$labName, function(model) {
    fileName <- allTests$fullName[allTests$modelName == model & allTests$testName == test]
    if(length(fileName)) {
      print(paste("Reading model", model, "test", test))
      dset <- read.dta(paste("../data/prepped/", fileName, sep = ""))
      dset$time <- stata_time_to_R_time(dset$time)
      dset <- dset[, c("time", "T_Tank_02", "T_Tank_04", "T_Tank_06", "T_Tank_08",
                       "T_Tank_10", "T_Tank_12", "Power_W", "Power_res_kW",
                       "Power_comp_kW", "flow_out_gpm", "T_In_water",
                       "T_Out_water", "minutes", "hours", "Power_kW",
                       "T_Tank_ave", "T_Tank_COP", "flow_out_gal", "dEadd5min")]
      dset$test <- test
      dset$model <- modelsToUse$simName[modelsToUse$labName == model]
      dset 
    } else {
      NULL
    }
  }))
}))

sort(unique(paste(allLabResults$test, allLabResults$model)))


names(allLabResults)[names(allLabResults) == "flow_out_gal"] <- "flow"
names(allLabResults)[names(allLabResults) == "T_Tank_02"] <- "tcouples1"
names(allLabResults)[names(allLabResults) == "T_Tank_04"] <- "tcouples2"
names(allLabResults)[names(allLabResults) == "T_Tank_06"] <- "tcouples3"
names(allLabResults)[names(allLabResults) == "T_Tank_08"] <- "tcouples4"
names(allLabResults)[names(allLabResults) == "T_Tank_10"] <- "tcouples5"
names(allLabResults)[names(allLabResults) == "T_Tank_12"] <- "tcouples6"
allLabResults$inputPower <- allLabResults$Power_W
allLabResults$outputPower <- allLabResults$dEadd5min / 5 / 60 * 1000

tempVars <- grep("tcouples", names(allLabResults))
allLabResults[, tempVars] <- allLabResults[, tempVars] * 1.8 + 32
allLabResults$aveTankTemp <- apply(allLabResults[, tempVars], 1, mean)

allLabResults <- allLabResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                             "tcouples1", "tcouples2", "tcouples3",
                             "tcouples4", "tcouples5", "tcouples6",
                             "aveTankTemp")]
allLabResults$type <- "Measured"

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

write.csv(file = "HpwhTestTool/allLabResults.csv", allLabResults, row.names = FALSE)




