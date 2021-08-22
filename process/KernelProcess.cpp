/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#include <KernelProcess.h>
#include <ProcessEnv.h>
#include <UserManager.h>
#include <ProcessManager.h>
#include <KernelThread.h>
#include <GraphicsVideo.h>
#include <DMM.h>

upan::mutex KernelProcess::_envMutex;

KernelProcess::KernelProcess(const upan::string& name, uint32_t taskAddress, int parentID, bool isFGProcess, const upan::vector<uint32_t>& params)
    : AutonomousProcess(name, parentID, isFGProcess), _iodTable(_processID, parentID) {
  _mainThreadID = _processID;
  ProcessEnv_InitializeForKernelProcess() ;
  _processBase = GLOBAL_DATA_SEGMENT_BASE;
  const uint32_t uiStackAddress = AllocateAddressSpace();
  const uint32_t uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1;
  _taskState.BuildForKernel(taskAddress, uiStackTop, params);
  _processLDT.BuildForKernel();
  _userID = ROOT_USER_ID ;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
}

KernelThread& KernelProcess::CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) {
  return *new KernelThread(*this, threadCaller, entryAddress, arg);
}

uint32_t KernelProcess::AllocateAddressSpace() {
  kernelStackBlockId = MemManager::Instance().AllocateKernelStack();
  return MemManager::Instance().GetKernelStackAddress(kernelStackBlockId);
}

void KernelProcess::DeAllocateResources() {
  MemManager::Instance().DeAllocateKernelStack(kernelStackBlockId);
  DeAllocateGUIFramebuffer();
}

void KernelProcess::initGuiFrame() {
  if (_frame.get() == nullptr) {
    FrameBufferInfo frameBufferInfo;
    const auto f = MultiBoot::Instance().VideoFrameBufferInfo();
    frameBufferInfo._pitch = f->framebuffer_pitch;
    frameBufferInfo._width = f->framebuffer_width;
    frameBufferInfo._height = f->framebuffer_height;
    frameBufferInfo._bpp = f->framebuffer_bpp;
    frameBufferInfo._frameBuffer = (uint32_t*)GraphicsVideo::Instance().allocateFrameBuffer();
    upanui::FrameBuffer frameBuffer(frameBufferInfo);
    upanui::Viewport viewport(0, 0, frameBufferInfo._width, frameBufferInfo._height);
    _frame.reset(new RootFrame(frameBuffer, viewport));

    //let processes write to stdout of the parent for debugging purpose.
    //_iodTable.setupNullStdOut();

    GraphicsVideo::Instance().addFGProcess(_processID);
  }
}

void KernelProcess::DeAllocateGUIFramebuffer() {
  if (_frame.get() != nullptr) {
    DMM_DeAllocateForKernel((uint32_t)_frame->frameBuffer().buffer());
    GraphicsVideo::Instance().removeFGProcess(_processID);
  }
}
