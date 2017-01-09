/*
 * Copyright (c) 2012, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "topological_map_panel.h"

#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>

#include "rviz/visualization_manager.h"

namespace rviz_topmap
{
Pan::Pan( QWidget* parent )
  : Panel( parent )
  , view_man_( NULL )
{
  camera_type_selector_ = new QComboBox;
  properties_view_ = new rviz::PropertyTreeWidget();

  save_button_ = new QPushButton( "Save" );
  QPushButton* remove_button = new QPushButton( "Remove" );
  QPushButton* rename_button = new QPushButton( "Rename" );
  QPushButton* zero_button = new QPushButton( "Zero" );
  zero_button->setToolTip( "Jump to 0,0,0 with the current view controller. Shortcut: Z" );

  QHBoxLayout* top_layout = new QHBoxLayout;
  top_layout->addWidget( new QLabel( "Type:" ));
  top_layout->addWidget( camera_type_selector_ );
  top_layout->addStretch();
  top_layout->addWidget( zero_button );
  top_layout->setContentsMargins( 2, 6, 2, 2 );

  QHBoxLayout* button_layout = new QHBoxLayout;
  button_layout->addWidget( save_button_ );
  button_layout->addWidget( remove_button );
  button_layout->addWidget( rename_button );
  button_layout->setContentsMargins( 2, 0, 2, 2 );

  QVBoxLayout* main_layout = new QVBoxLayout;
  main_layout->setContentsMargins( 0,0,0,0 );
  main_layout->addLayout( top_layout );
  main_layout->addWidget( properties_view_ );
  main_layout->addLayout( button_layout );
  setLayout( main_layout );

  connect( remove_button, SIGNAL( clicked() ), this, SLOT( onDeleteClicked() ));
  connect( rename_button, SIGNAL( clicked() ), this, SLOT( renameSelected() ));
  connect( zero_button, SIGNAL( clicked() ), this, SLOT( onZeroClicked() ));
  connect( properties_view_, SIGNAL( clicked( const QModelIndex& )), this, SLOT( setCurrentViewFromIndex( const QModelIndex& )));
  connect( properties_view_, SIGNAL( activated( const QModelIndex& )), this, SLOT( setCurrentViewFromIndex( const QModelIndex& )));
}

void Pan::onInitialize()
{
}

void Pan::setMan( Man* view_man )
{
  if( view_man_ )
  {
    disconnect( save_button_, SIGNAL( clicked() ), view_man_, SLOT( copyCurrentToList() ));
    disconnect( camera_type_selector_, SIGNAL( activated( int )), this, SLOT( onTypeSelectorChanged( int )));
    disconnect( view_man_, SIGNAL( currentChanged() ), this, SLOT( onCurrentChanged() ));
  }
  view_man_ = view_man;
  camera_type_selector_->clear();
  if( view_man_ )
  {
    properties_view_->setModel( view_man_->getPropertyModel() );

    QStringList ids = view_man_->getFactory()->getDeclaredClassIds();
    for( int i = 0; i < ids.size(); i++ )
    {
      const QString& id = ids[ i ];
      camera_type_selector_->addItem( MyController::formatClassId( id ), id ); // send the regular-formatted id as userData.
    }

    connect( save_button_, SIGNAL( clicked() ), view_man_, SLOT( copyCurrentToList() ));
    connect( camera_type_selector_, SIGNAL( activated( int )), this, SLOT( onTypeSelectorChanged( int )));
    connect( view_man_, SIGNAL( currentChanged() ), this, SLOT( onCurrentChanged() ));
  }
  else
  {
    properties_view_->setModel( NULL );
  }
  onCurrentChanged();
}

void Pan::onTypeSelectorChanged( int selected_index )
{
  QString class_id = camera_type_selector_->itemData( selected_index ).toString();
  view_man_->setCurrentMyControllerType( class_id );
}

void Pan::onZeroClicked()
{
  if( view_man_->getCurrent() )
  {
    view_man_->getCurrent()->reset();
  }
}

void Pan::setCurrentViewFromIndex( const QModelIndex& index )
{
  rviz::Property* prop = view_man_->getPropertyModel()->getProp( index );
  if( MyController* view = qobject_cast<MyController*>( prop ))
  {
    view_man_->setCurrentFrom( view );
  }
}

void Pan::onDeleteClicked()
{
  QList<MyController*> views_to_delete = properties_view_->getSelectedObjects<MyController>();

  for( int i = 0; i < views_to_delete.size(); i++ )
  {
    // TODO: should eventually move to a scheme where the CURRENT view
    // is not in the same list as the saved views, at which point this
    // check can go away.
    if( views_to_delete[ i ] != view_man_->getCurrent() )
    {
      delete views_to_delete[ i ];
    }
  }
}

void Pan::renameSelected()
{
  QList<MyController*> views_to_rename = properties_view_->getSelectedObjects<MyController>();
  if( views_to_rename.size() == 1 )
  {
    MyController* view = views_to_rename[ 0 ];

    // TODO: should eventually move to a scheme where the CURRENT view
    // is not in the same list as the saved views, at which point this
    // check can go away.
    if( view == view_man_->getCurrent() )
    {
      return;
    }

    QString old_name = view->getName();
    QString new_name = QInputDialog::getText( this, "Rename View", "New Name?", QLineEdit::Normal, old_name );

    if( new_name.isEmpty() || new_name == old_name )
    {
      return;
    }

    view->setName( new_name );
  }
}

void Pan::onCurrentChanged()
{
  QString formatted_class_id = MyController::formatClassId( view_man_->getCurrent()->getClassId() );

  // Make sure the type selector shows the type of the current view.
  // This is only here in case the type is changed programmatically,
  // instead of via the camera_type_selector_ being used.
  camera_type_selector_->setCurrentIndex( camera_type_selector_->findText( formatted_class_id ));

  properties_view_->setAnimated( false );
  view_man_->getCurrent()->expand();
  properties_view_->setAnimated( true );
}

void Pan::save( rviz::Config config ) const
{
  rviz::Panel::save( config );
  properties_view_->save( config );
}

void Pan::load( const rviz::Config& config )
{
  rviz::Panel::load( config );
  properties_view_->load( config );
}

} // namespace rviz_topmap
