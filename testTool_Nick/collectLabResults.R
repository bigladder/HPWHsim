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

COPtests <- grep("^COP_[0-9]+$", allTests$testName)
aggregate(modelName ~ testName, FUN = length, data = allTests[COPtests, ])
table(allTests$testName[COPtests], allTests$modelName[COPtests])

testsToUse <- c("DOE_24hr50", "DOE_24hr67", "DP_SHW50",
                "DOE2014_24hr67", "DOE2014_24hr50")
modelsToUse <- data.frame("labName" = c("ATI66rev2", "GE502014", "GE502014STDMode", "GEred", "GE",
                                        "SandenGES", "AOSmith60", "AOSmith80"),
                          "simName" = c("ATI66", "GE502014", "GE502014STDMode", "GEred", "GE", 
                                        "Sanden80", "Voltex60", "Voltex80"),
                          stringsAsFactors = FALSE)

# Read the attributes file...
hpwhAttr <- read.dta("../data/attributes.dta")
names(hpwhAttr)[1] <- "labName"
hpwhAttr <- merge(hpwhAttr, modelsToUse)


readOne <- function(model, test) {
  fileName <- allTests$fullName[allTests$modelName == model & allTests$testName == test]
  if(length(fileName)) {
    print(paste("Reading model", model, "test", test))
    dset <- read.dta(paste("../data/prepped/", fileName, sep = ""))
    dset$time <- stata_time_to_R_time(dset$time)
    dset <- dset[, names(dset) %in% c("time", "T_Tank_02", "T_Tank_04", "T_Tank_06", "T_Tank_08",
                     "T_Tank_10", "T_Tank_12", "Power_W", "Power_res_kW",
                     "Power_comp_kW", "flow_out_gpm", "T_In_water",
                     "T_Out_water", "minutes", "Power_kW",
                     "T_Tank_ave", "T_Tank_COP", "flow_out_gal", "dEadd5min")]
    dset$test <- test
    simName <- modelsToUse$simName[modelsToUse$labName == model]
    if(length(simName)) {
      dset$model <- simName
    } else {
      dset$model <- model
    }
    dset 
  } else {
    NULL
  }
}

allLabResults <- do.call('rbind', lapply(testsToUse, function(test) {
  do.call('rbind', lapply(modelsToUse$labName, readOne, test))
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

# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
# setwd("Z:/Documents/HPWH/HPWHsim/testTool/")
setwd("/storage/server/nkvaltine/Projects/HPWHsim/testTool/")

write.csv(file = "HpwhTestTool/allLabResults.csv", allLabResults, row.names = FALSE)


write.csv(file = "HpwhTestTool/hpwhProperties.csv", hpwhAttr, row.names = FALSE)

# 
# 
# # Read all the COP_67 tests....
# testName <- "COP_67"
# cop67Models <- allTests$modelName[allTests$testName == "COP_67"]
# 
# cop67 <- do.call('rbind', lapply(cop67Models, readOne, "COP_67"))
# 
# cop67$T_Tank_ave <- cop67$T_Tank_ave * 1.8 + 32
# ggplot(cop67[cop67$minutes < 400, ]) + theme_bw() + 
#   geom_line(aes(minutes, T_Tank_ave, col = model))


