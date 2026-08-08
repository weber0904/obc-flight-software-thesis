#include "SequenceAdmissionControllerTester.hpp"

TEST(SequenceAdmissionController, ValidateAcceptsBackupReadableSequence) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testValidateAcceptsBackupReadableSequence();
}

TEST(SequenceAdmissionController, RunRejectsBackupHighAuthoritySequence) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testRunRejectsBackupHighAuthoritySequence();
}

TEST(SequenceAdmissionController, PrepareManualUsesAdmittedCopyAndAllowsOwnedStart) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testPrepareManualUsesAdmittedCopyAndAllowsOwnedStart();
}

TEST(SequenceAdmissionController, ManualCompletionRestoresAutoMode) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testManualCompletionRestoresAutoMode();
}

TEST(SequenceAdmissionController, ManualCancelRestoresAutoMode) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testManualCancelRestoresAutoMode();
}

TEST(SequenceAdmissionController, ManualLoadFailureRestoresAutoMode) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testManualLoadFailureRestoresAutoMode();
}

TEST(SequenceAdmissionController, AutoRestoreFailureIsRetried) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testAutoRestoreFailureIsRetried();
}

TEST(SequenceAdmissionController, AdmissionCopyFailureCleansUpAdmittedPath) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testAdmissionCopyFailureCleansUpAdmittedPath();
}

TEST(SequenceAdmissionController, OwnerMismatchRejectsControl) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testOwnerMismatchRejectsControl();
}

TEST(SequenceAdmissionController, StartAndStepRejectAutoRunContext) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testStartAndStepRejectAutoRunContext();
}

TEST(SequenceAdmissionController, ManualStartAndStepFailuresRestoreAutoMode) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testManualStartAndStepFailuresRestoreAutoMode();
}

TEST(SequenceAdmissionController, RunLifecycleUpdatesFailureState) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testRunLifecycleUpdatesFailureState();
}

TEST(SequenceAdmissionController, RunFailureBeforeSeqStartUsesPredictedSequencer) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testRunFailureBeforeSeqStartUsesPredictedSequencer();
}

TEST(SequenceAdmissionController, TerminalContextsRejectFurtherControl) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testTerminalContextsRejectFurtherControl();
}

TEST(SequenceAdmissionController, TerminalContextsAreReused) {
    OBC::SequenceAdmissionControllerTester tester;
    tester.testTerminalContextsAreReused();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
